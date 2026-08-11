/*
Führt die eigentliche Arbeit aus (z.B. Lesen, Filtern, Schreiben). Ein Worker verarbeitet Daten, wendet Filter an und kommuniziert mit Supervisor/Pipes.
*/



#include "worker.hpp"

#include <iostream>
#include <unistd.h>

void worker(
    int channel,
    int read_fd,
    int write_fd,
    const FilterChain& filter_chain)
{
    unsigned char sample;
    unsigned char processed_sample;


    std::cout
        << "Worker "
        << channel
        << " gestartet\n";


    while(read(read_fd, &sample, 1) > 0)
    {
        std::cout
            << "Worker "
            << channel
            << " empfängt: "
            << (int)sample
            << std::endl;

        processed_sample = applyFilterChain(filter_chain, sample);

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


    close(read_fd);
    close(write_fd);
}
