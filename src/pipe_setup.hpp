/*
Deklariert die Funktionen zum Erstellen und Verwalten der Pipes, die von anderen Modulen genutzt werden.
*/

#pragma once

#include "splitter.hpp"

bool createPipeSet(int pipes[CHANNELS][2]);
void closePipeSet(int pipes[CHANNELS][2]);