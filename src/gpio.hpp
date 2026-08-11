/*
Stellt die Schnittstelle zu den GPIO-Funktionen bereit (was andere Komponenten aufrufen können), z.B. zum Setzen oder Abfragen von Pins.
*/

#pragma once

#include <cstddef>

bool initGpios();
void pulseGpio(std::size_t channel);
void closeGpios();