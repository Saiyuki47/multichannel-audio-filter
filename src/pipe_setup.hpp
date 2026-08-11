/*
Deklariert die Funktionen zum Erstellen und Verwalten der Pipes, die von anderen Modulen genutzt werden.
*/

#pragma once

#include "splitter.hpp"

// Legt für alle Kanäle je eine Pipe an. Gibt true zurück, wenn alles klappt.
bool createPipeSet(int pipes[CHANNELS][2]);
// Schließt alle Enden aller Pipes wieder.
void closePipeSet(int pipes[CHANNELS][2]);
