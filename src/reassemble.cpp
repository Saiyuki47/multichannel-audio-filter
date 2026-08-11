/*
Nimmt aufgeteilte Daten-Chunks und setzt sie wieder zu vollständigen Samples zusammen. Verantwortlich für das korrekte Zusammenfügen von Segmenten in die Originalfolge.

Diese Datei arbeitet mit fertigen DATEIEN pro Kanal (nicht mit Pipes). Sie liest
reihum aus jedem Kanal ein Sample und schreibt es in die Ausgabedatei – also
Kanal1, Kanal2, Kanal3, Kanal4, Kanal1, ... So entsteht wieder die
ursprüngliche Reihenfolge.
*/


#include "reassemble.hpp"


#include <fstream>
#include <iostream>

void assembleOutput(const std::string channel_outputs[4], const std::string& final_output_path)
{
    std::ifstream inputs[4]; // die 4 Kanal-Dateien zum Lesen

    // Die Ausgabedatei zum Schreiben öffnen. "trunc" = vorher leeren.
    std::ofstream output(
        final_output_path,
        std::ios::binary | std::ios::trunc);


    // Klappt das Öffnen der Ausgabe nicht, brechen wir ab.
    if(!output)
    {
        std::cerr
            << "Kann Ausgabedatei nicht oeffnen: "
            << final_output_path
            << std::endl;

        return;
    }


    // Alle vier Kanal-Dateien öffnen.
    for(int i = 0; i < 4; i++)
    {
        inputs[i].open(
            channel_outputs[i],
            std::ios::binary);


        // Fehlt eine Kanaldatei, abbrechen.
        if(!inputs[i])
        {
            std::cerr
                << "Kann Kanaldatei nicht oeffnen: "
                << channel_outputs[i]
                << std::endl;

            return;
        }
    }


    unsigned char sample;
    bool wrote_data = true; // Merker: haben wir in dieser Runde noch etwas geschrieben?


    // Immer eine komplette Runde über alle 4 Kanäle machen,
    // so lange in mindestens einem Kanal noch Daten waren.
    while(wrote_data)
    {
        wrote_data = false;


        for(int i = 0; i < 4; i++)
        {
            // Aus Kanal i ein Sample lesen ...
            if(inputs[i].read(
                   reinterpret_cast<char*>(&sample),
                   1))
            {
                // ... und sofort in die Ausgabe schreiben.
                output.write(
                    reinterpret_cast<const char*>(&sample),
                    1);

                wrote_data = true; // es kam noch was -> weiter machen
            }
        }
    }
}
