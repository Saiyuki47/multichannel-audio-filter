/*
Führt die eigentliche Arbeit aus (z.B. Lesen, Filtern, Schreiben). Ein Worker verarbeitet Daten, wendet Filter an und kommuniziert mit Supervisor/Pipes.
*/



#include "worker.hpp"

#include <iostream>
#include <unistd.h>

// Ein Worker läuft als eigener Prozess und macht immer dasselbe:
// ein Sample lesen -> filtern -> das Ergebnis weiterschreiben.
void worker(
    int channel,
    int read_fd,
    int write_fd,
    const FilterChain& filter_chain)
{
    unsigned char sample;           // das gerade gelesene Roh-Sample
    unsigned char processed_sample; // das gefilterte Sample


    std::cout
        << "Worker "
        << channel
        << " gestartet\n";


    // read(...) liefert die Anzahl gelesener Bytes.
    // Solange noch etwas kommt (> 0), weitermachen.
    // Kommt 0, ist die Pipe geschlossen (EOF) -> Schleife endet.
    while(read(read_fd, &sample, 1) > 0)
    {
        std::cout
            << "Worker "
            << channel
            << " empfängt: "
            << (int)sample
            << std::endl;

        // Die komplette Filterkette dieses Kanals auf das Sample anwenden.
        processed_sample = applyFilterChain(filter_chain, sample);

        // Das fertige Sample in die Ausgabe-Pipe schreiben.
        write(
            write_fd,
            &processed_sample,
            1
        );


        // später:
        // DSP Verarbeitung
        // GPIO Ausgabe
    }


    std::cout
        << "Worker "
        << channel
        << " beendet\n";


    // Beide Pipe-Enden schließen, wir brauchen sie nicht mehr.
    close(read_fd);
    close(write_fd);
}
