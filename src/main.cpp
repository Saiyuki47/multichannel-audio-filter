/*
Einstiegspunkt des Programms. Parst Kommandozeilenargumente, initialisiert Komponenten (Filter, GPIO, Pipes) und startet den Hauptablauf oder Supervisor/Worker.
*/

#include "supervisor.hpp"

// Hier startet das Programm. Wir machen selbst fast nichts,
// sondern übergeben direkt an den Supervisor, der alles Weitere
// (Pipes, Worker-Prozesse, Splitter, Zusammenbau) organisiert.
int main()
{
    return runAudioSystem();
}
