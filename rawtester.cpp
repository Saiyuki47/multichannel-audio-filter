/*
Kleines Test-Werkzeug: prüft, ob eine Ausgabedatei wirklich das Ergebnis ist,
das man erwartet, wenn man die Filterketten auf die Eingabedatei anwendet.

Idee: Beide Dateien Byte für Byte durchgehen. Für jedes Eingabe-Byte selbst den
Filter rechnen und mit dem Ausgabe-Byte vergleichen. Passt alles -> "OK".
Passt etwas nicht -> Fehlermeldung mit genauer Stelle.

Aufruf: rawtester <input.raw> <output.raw> [Filter/filters.json]
*/

#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

#include "src/filterJSON.hpp"
#include "src/filters.hpp"

namespace
{
constexpr std::size_t CHANNEL_COUNT = 4;

// Liest ein einzelnes Byte aus dem Stream.
// Rückgabe true = ein Byte gelesen, false = Dateiende (oder Fehler).
bool readByte(std::ifstream& stream, unsigned char& byte)
{
    return static_cast<bool>(stream.read(reinterpret_cast<char*>(&byte), 1));
}
} // namespace

int main(int argc, char* argv[])
{
    // Es müssen mindestens Eingabe- und Ausgabedatei angegeben sein.
    if(argc < 3)
    {
        std::cerr
            << "Verwendung: "
            << argv[0]
            << " <input.raw> <output.raw> [Filter/filters.json]\n";

        return 1;
    }

    const char* input_path = argv[1];
    const char* output_path = argv[2];
    // Dritter Parameter ist optional; sonst die Standard-Konfiguration.
    const std::string config_path = argc >= 4 ? argv[3] : "Filter/filters.json";

    // Dieselben Filterketten laden, die auch das echte Programm benutzt.
    ChannelFilterChains channel_filters = loadFilterChains(config_path);

    std::ifstream input(input_path, std::ios::binary);
    if(!input)
    {
        std::cerr << "Kann Input-Datei nicht oeffnen: " << input_path << "\n";
        return 1;
    }

    std::ifstream output(output_path, std::ios::binary);
    if(!output)
    {
        std::cerr << "Kann Output-Datei nicht oeffnen: " << output_path << "\n";
        return 1;
    }

    unsigned char input_byte = 0;
    unsigned char output_byte = 0;
    std::size_t index = 0; // das wievielte Sample wir gerade prüfen

    while(true)
    {
        // Aus beiden Dateien gleichzeitig ein Byte lesen.
        bool input_ok = readByte(input, input_byte);
        bool output_ok = readByte(output, output_byte);

        // Endet eine Datei früher als die andere, stimmt die Länge nicht.
        if(input_ok != output_ok)
        {
            std::cerr << "Dateilaenge passt nicht: Vergleich bricht bei Sample " << index << " ab\n";
            return 2;
        }

        // Beide am Ende -> alles geprüft, fertig.
        if(!input_ok)
            break;

        // Zu welchem Kanal gehört dieses Sample? (reihum 0..3)
        std::size_t channel = index % CHANNEL_COUNT;
        // Selbst ausrechnen, was herauskommen müsste.
        unsigned char expected_byte = applyFilterChain(channel_filters[channel], input_byte);

        // Stimmt das erwartete Byte nicht mit dem echten überein -> melden.
        if(output_byte != expected_byte)
        {
            std::cerr
                << "Mismatch bei Sample "
                << index
                << " (Kanal "
                << (channel + 1)
                << "): Input="
                << static_cast<unsigned int>(input_byte)
                << ", Erwartet="
                << static_cast<unsigned int>(expected_byte)
                << ", Ist="
                << static_cast<unsigned int>(output_byte)
                << "\n";

            return 2;
        }

        ++index;
    }

    std::cout
        << "OK: "
        << index
        << " Samples geprueft, Input und Output passen zu den Filterketten.\n";

    return 0;
}
