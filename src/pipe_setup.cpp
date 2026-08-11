/*
Richtet Kommunikationskanäle (Pipes) zwischen Prozessen/Threads ein. Diese Datei erstellt und konfiguriert die Verbindungen, über die Daten weitergegeben werden.

Kurz gesagt: Eine Pipe ist eine Röhre mit zwei Enden. In [1] schreibt man rein,
aus [0] liest man wieder heraus. So reden die Prozesse miteinander.
*/

#include "pipe_setup.hpp"

#include <unistd.h>

// Legt für jeden Kanal eine eigene Pipe an.
bool createPipeSet(int pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        // pipe(...) füllt pipes[i][0] (lesen) und pipes[i][1] (schreiben).
        // Rückgabe -1 bedeutet: hat nicht geklappt.
        if(pipe(pipes[i]) == -1)
        {
            // Aufräumen: die bereits erzeugten Pipes wieder schließen,
            // damit keine offenen Enden übrig bleiben.
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

// Schließt alle Enden aller Pipes.
void closePipeSet(int pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
}
