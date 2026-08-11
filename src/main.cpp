/*
Einstiegspunkt des Programms. Parst Kommandozeilenargumente, initialisiert Komponenten (Filter, GPIO, Pipes) und startet den Hauptablauf oder Supervisor/Worker.
*/

#include "supervisor.hpp"

int main()
{
    return runAudioSystem();
}