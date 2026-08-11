/*
Deklariert die Funktionen und Typen für Worker, sodass Supervisor oder Startcode Worker-Instanzen erzeugen und verwalten kann.
*/

#pragma once

#include <string>

#include "filters.hpp"

// Ein Worker kümmert sich um genau EINEN Kanal.
// channel      = Nummer des Kanals (nur für die Ausgabe / Info)
// read_fd      = Pipe-Ende, aus dem der Worker rohe Samples liest
// write_fd     = Pipe-Ende, in das der Worker gefilterte Samples schreibt
// filter_chain = die Filter, die dieser Kanal anwenden soll
void worker(
    int channel,
    int read_fd,
    int write_fd,
    const FilterChain& filter_chain);
