#pragma once

// Wie viele Audio-Kanäle wir verarbeiten. Der Wert wird im ganzen
// Programm benutzt (Anzahl Pipes, Anzahl Worker usw.).
constexpr int CHANNELS = 4;

// Der Splitter liest die Eingabedatei und verteilt die Samples
// reihum auf die 4 Kanal-Pipes.
void splitter(int pipes[CHANNELS][2]);
