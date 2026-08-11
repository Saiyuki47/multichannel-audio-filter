/*
 Deklariert die Reassemble-Funktionen und benötigte Typen, damit andere Teile die Wiederzusammenführung von Daten nutzen können.
*/

#pragma once

#include <string>

void assembleOutput(const std::string channel_outputs[4], const std::string& final_output_path);
