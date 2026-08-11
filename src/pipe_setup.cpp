/*
Richtet Kommunikationskanäle (Pipes) zwischen Prozessen/Threads ein. Diese Datei erstellt und konfiguriert die Verbindungen, über die Daten weitergegeben werden.
*/

#include "pipe_setup.hpp"

#include <unistd.h>

bool createPipeSet(int pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        if(pipe(pipes[i]) == -1)
        {
            for(int j = 0; j <= i; ++j)
            {
                close(pipes[j][0]);
                close(pipes[j][1]);
            }

            return false;
        }
    }

    return true;
}

void closePipeSet(int pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}