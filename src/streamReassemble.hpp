#pragma once

#include <string>

// Live-Variante des Zusammensetzens: liest die gefilterten Samples direkt aus
// den vier Kanal-PIPES (nicht aus Dateien) und schreibt sie reihum in die
// Ausgabedatei. Passt so gut zu einem laufenden Datenstrom.
void assembleOutputFromPipes(int pipes[4][2], const std::string& final_output_path);
