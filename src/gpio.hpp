/*
Stellt die Schnittstelle zu den GPIO-Funktionen bereit (was andere Komponenten aufrufen können), z.B. zum Setzen oder Abfragen von Pins.

GPIO = "General Purpose Input/Output", also die Ein-/Ausgangs-Pins auf einem
Board (hier ein Odroid C4). Wir benutzen sie als Ausgänge, um pro Kanal einen
kurzen Impuls auszugeben.
*/

#pragma once

#include <cstddef>

// Richtet alle GPIO-Ausgänge ein. true = hat geklappt.
bool initGpios();
// Gibt auf dem Pin des angegebenen Kanals einen kurzen Impuls aus.
void pulseGpio(std::size_t channel);
// Gibt die GPIO-Ressourcen wieder frei.
void closeGpios();
