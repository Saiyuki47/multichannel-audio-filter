/*
Liest die Eingabe-RAW-Datei und verteilt die einzelnen Samples im Round Robin Verfahren reihum auf die
vier Kanal-Pipes (Kanal 1, 2, 3, 4, 1, 2, 3, 4, ...). Jeder Worker bekommt so
jedes vierte Sample.
*/

#include "splitter.hpp"

#include <fstream>
#include <iostream>
#include <unistd.h>


void splitter(int pipes[CHANNELS][2])
{
    std::cout
        << "Splitter gestartet\n";


    // Der Splitter schreibt nur, er liest nicht aus den Pipes.
    // Deshalb die lese-Enden ([0]) schließen; die schreib-Enden ([1]) behalten.
    for(int i = 0; i < CHANNELS; i++)
    {
        close(pipes[i][0]);
    }


    // Die Roh-Audiodatei öffnen (binär, also Byte für Byte).
    std::ifstream input(
        "input/audio_data_team1.raw",
        std::ios::binary);


    // Falls die Datei fehlt: Fehlermeldung, Pipes schließen und aufhören.
    if(!input)
    {
        std::cerr
            << "Kann input/audio_data_team1.raw nicht oeffnen\n";

        for(int i = 0; i < CHANNELS; i++)
        {
            close(pipes[i][1]);
        }

        return;
    }


    unsigned char sample; // das gerade gelesene Byte
    int index = 0;        // Zähler, das wievielte Sample es ist


    // Die Datei Byte für Byte durchlesen, bis nichts mehr kommt.
    while(input.read(
        reinterpret_cast<char*>(&sample),
        1))
    {
        // Reihum verteilen: 0->Kanal0, 1->Kanal1, ... 4->Kanal0 usw.
        int channel = index % CHANNELS;


        // Das Sample in die Pipe des passenden Kanals schreiben.
        write(
            pipes[channel][1],
            &sample,
            1);


        index++;

        // Kurze Pause (0,2 s), damit man die Verarbeitung "live" mitverfolgen
        // kann (simuliert einen Datenstrom in Echtzeit).
        usleep(200000);
    }


    std::cout
        << "Splitter fertig\n";


    // Zum Schluss die schreib-Enden schließen.
    // Dadurch bekommen die Worker ein EOF und wissen: es kommt nichts mehr.
    for(int i = 0; i < CHANNELS; i++)
    {
        close(pipes[i][1]);
    }
}
