
/*
Kümmert sich um das Ein- und Ausschalten oder Abfragen von GPIO-Pins (Hardware-Ein-/Ausgänge). Dieses Modul kapselt die plattformspezifischen Details der GPIO-Nutzung.
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
constexpr const char* GPIO_CHIP_PATH = "/dev/gpiochip0";
constexpr useconds_t PULSE_US = 50;

// Odroid C4 header pins mapped from `gpioinfo`.
// These are chip-relative line offsets on gpiochip0.
constexpr std::array<int, CHANNELS> GPIO_LINES = {
    63, // PIN_27
    64, // PIN_28
    65, // PIN_16
    66  // PIN_18
};

std::array<gpiod_line*, CHANNELS> gpioLines{};
gpiod_chip* gpioChip = nullptr;

bool requestLine(
    const std::array<int, CHANNELS>& lines,
    std::array<gpiod_line*, CHANNELS>& storage,
    std::size_t channel,
    const char* consumer,
    bool required
)
{
    if(channel >= CHANNELS)
        return false;

    if(gpioChip == nullptr)
    {
        gpioChip = gpiod_chip_open(GPIO_CHIP_PATH);
        if(gpioChip == nullptr)
        {
            std::cerr << "Kann " << GPIO_CHIP_PATH << " nicht oeffnen" << std::endl;
            return false;
        }
    }

    auto* line = gpiod_chip_get_line(gpioChip, lines[channel]);
    if(line == nullptr)
    {
        std::cerr << "Kann GPIO-Line " << lines[channel] << " nicht anfordern" << std::endl;
        return !required;
    }

    if(gpiod_line_request_output(line, consumer, 0) != 0)
    {
        std::cerr << "Kann GPIO-Line " << lines[channel] << " nicht als Ausgang konfigurieren" << std::endl;
        std::cerr << "Grund: " << std::strerror(errno) << std::endl;
        gpiod_line_release(line);
        return !required;
    }

    storage[channel] = line;
    return true;
}
} // namespace

bool initGpios()
{
    for(std::size_t channel = 0; channel < CHANNELS; ++channel)
    {
        if(!requestLine(GPIO_LINES, gpioLines, channel, "restart-pulse", true))
        {
            closeGpios();
            return false;
        }
    }

    return true;
}

void pulseGpio(std::size_t channel)
{
    if(channel >= CHANNELS)
        return;

    auto* line = gpioLines[channel];
    if(line == nullptr)
        return;

    if(gpiod_line_set_value(line, 1) != 0)
        return;

    usleep(PULSE_US);
    gpiod_line_set_value(line, 0);
}

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