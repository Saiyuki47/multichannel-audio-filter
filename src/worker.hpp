/*
Deklariert die Funktionen und Typen für Worker, sodass Supervisor oder Startcode Worker-Instanzen erzeugen und verwalten kann.
*/

#pragma once

#include <string>

#include "filters.hpp"

void worker(
    int channel,
    int read_fd,
    int write_fd,
    const FilterChain& filter_chain);
