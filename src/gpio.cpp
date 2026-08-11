
/*
Kümmert sich um das Ein- und Ausschalten oder Abfragen von GPIO-Pins (Hardware-Ein-/Ausgänge). Dieses Modul kapselt die plattformspezifischen Details der GPIO-Nutzung.

Benutzt wird die Bibliothek libgpiod (#include <gpiod.h>). Ablauf immer gleich:
Chip öffnen -> einzelne Leitungen ("lines") anfordern -> Werte setzen ->
am Ende alles wieder freigeben.
*/

#include "gpio.hpp"

#include <array>
#include <cerrno>
#include <iostream>
#include <cstring>
#include <unistd.h>


#include <gpiod.h>


namespace
{
constexpr std::size_t CHANNELS = 4;
// Der GPIO-Chip als Gerätedatei im System.
constexpr const char* GPIO_CHIP_PATH = "/dev/gpiochip0";
// Wie lange ein Impuls "an" ist (in Mikrosekunden). 50 = sehr kurz.
constexpr useconds_t PULSE_US = 50;

// Odroid C4 header pins mapped from `gpioinfo`.
// These are chip-relative line offsets on gpiochip0.
// Für jeden Kanal die passende Leitungsnummer auf dem Chip.
constexpr std::array<int, CHANNELS> GPIO_LINES = {
    63, // PIN_27
    64, // PIN_28
    65, // PIN_16
    66  // PIN_18
};

// Hier merken wir uns die angeforderten Leitungen und den geöffneten Chip.
// (Global, damit init/pulse/close alle darauf zugreifen können.)
std::array<gpiod_line*, CHANNELS> gpioLines{};
gpiod_chip* gpioChip = nullptr;

// Fordert eine einzelne GPIO-Leitung für einen Kanal an und stellt sie als
// Ausgang ein. "required" sagt, ob ein Fehler wirklich schlimm ist.
bool requestLine(
    const std::array<int, CHANNELS>& lines,
    std::array<gpiod_line*, CHANNELS>& storage,
    std::size_t channel,
    const char* consumer, // Name, unter dem wir die Leitung "belegen"
    bool required)
{
    // Ungültige Kanalnummer -> nichts tun.
    if(channel >= CHANNELS)
        return false;

    // Den Chip nur einmal öffnen (beim ersten Aufruf).
    if(gpioChip == nullptr)
    {
        gpioChip = gpiod_chip_open(GPIO_CHIP_PATH);
        if(gpioChip == nullptr)
        {
            std::cerr << "Kann " << GPIO_CHIP_PATH << " nicht oeffnen" << std::endl;
            return false;
        }
    }

    // Die konkrete Leitung vom Chip holen.
    auto* line = gpiod_chip_get_line(gpioChip, lines[channel]);
    if(line == nullptr)
    {
        std::cerr << "Kann GPIO-Line " << lines[channel] << " nicht anfordern" << std::endl;
        // Wenn nicht zwingend nötig, ist das kein harter Fehler.
        return !required;
    }

    // Leitung als Ausgang anfordern, Startwert 0 (aus).
    if(gpiod_line_request_output(line, consumer, 0) != 0)
    {
        std::cerr << "Kann GPIO-Line " << lines[channel] << " nicht als Ausgang konfigurieren" << std::endl;
        std::cerr << "Grund: " << std::strerror(errno) << std::endl;
        gpiod_line_release(line); // wieder freigeben, sonst bleibt sie belegt
        return !required;
    }

    // Erfolgreich angefordert -> merken.
    storage[channel] = line;
    return true;
}
} // namespace

// Richtet die GPIO-Ausgänge für alle Kanäle ein.
bool initGpios()
{
    for(std::size_t channel = 0; channel < CHANNELS; ++channel)
    {
        // Schlägt eine Pflicht-Leitung fehl, alles aufräumen und abbrechen.
        if(!requestLine(GPIO_LINES, gpioLines, channel, "restart-pulse", true))
        {
            closeGpios();
            return false;
        }
    }

    return true;
}

// Gibt auf dem Pin des Kanals einen kurzen Impuls: an -> warten -> aus.
void pulseGpio(std::size_t channel)
{
    // Ungültiger Kanal -> nichts tun.
    if(channel >= CHANNELS)
        return;

    auto* line = gpioLines[channel];
    if(line == nullptr) // Leitung wurde gar nicht eingerichtet
        return;

    // Pin auf 1 (an) setzen. Klappt das nicht, abbrechen.
    if(gpiod_line_set_value(line, 1) != 0)
        return;

    usleep(PULSE_US);              // ganz kurz warten
    gpiod_line_set_value(line, 0); // Pin wieder auf 0 (aus)
}

// Gibt alle Leitungen und den Chip wieder frei (Aufräumen am Ende).
void closeGpios()
{
    for(auto& line : gpioLines)
    {
        if(line != nullptr)
        {
            gpiod_line_release(line);
            line = nullptr;
        }
    }

    if(gpioChip != nullptr)
    {
        gpiod_chip_close(gpioChip);
        gpioChip = nullptr;
    }
}
