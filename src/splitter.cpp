#include "splitter.hpp"

#include <fstream>
#include <iostream>
#include <unistd.h>



void splitter(int pipes[CHANNELS][2])
{
    std::cout 
        << "Splitter gestartet\n";


    // Schreibende Enden behalten
    // Lesende Enden schließen

    for(int i=0;i<CHANNELS;i++)
    {
        close(pipes[i][0]);
    }


    std::ifstream input(
        "input/audio_data_team1.raw",
        std::ios::binary
    );
    

    if(!input)
    {
        std::cerr
            << "Kann input/audio_data_team1.raw nicht oeffnen\n";

        for(int i=0;i<CHANNELS;i++)
        {
            close(pipes[i][1]);
        }

        return;
    }


    unsigned char sample;
    int index = 0;


    while(input.read(
        reinterpret_cast<char*>(&sample),
        1))
    {
        int channel = index % CHANNELS;


        write(
            pipes[channel][1],
            &sample,
            1
        );


        index++;

        usleep(200000);
    }


    std::cout
        << "Splitter fertig\n";


    // Pipes schließen
    // signalisiert Workern EOF

    for(int i=0;i<CHANNELS;i++)
    {
        close(pipes[i][1]);
    }
}