/*
 Deklariert die Reassemble-Funktionen und benötigte Typen, damit andere Teile die Wiederzusammenführung von Daten nutzen können.
*/

#pragma once

#include <string>

// Variante, die aus vier fertigen Kanal-DATEIEN eine einzige Ausgabedatei baut.
// (Die Live-Variante über Pipes steht in streamReassemble.hpp.)
// channel_outputs   = die 4 Eingabe-Dateinamen (pro Kanal einer)
// final_output_path = wohin das zusammengesetzte Ergebnis geschrieben wird
void assembleOutput(const std::string channel_outputs[4], const std::string& final_output_path);
