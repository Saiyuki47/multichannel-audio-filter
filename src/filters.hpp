/*
Definiert die Filter-Schritte und die Signaturen der Filterfunktionen. Andere Module importieren hier die Filter-Funktionen und Datentypen.
*/
#pragma once

#include <vector>

// Ein Audio-Sample ist ein Byte (0..255). Die "Ruhe" bzw. Mittellinie liegt
// nicht genau bei 127, sondern bei diesem gemessenen Wert. Alle Filter rechnen
// relativ zu diesem Mittelpunkt.
constexpr double SAMPLE_MIDPOINT = 126.1;

// Ein einzelner Filter-Schritt. "type" sagt, WAS gemacht wird,
// "amount" ist der zugehörige Wert (z.B. wie viel Prozent lauter).
struct FilterStep
{
    enum class Type
    {
        IncreaseVolume, // Sample lauter machen (verstärken)
        Absolute        // Absolutwert um die Mittellinie bilden
    } type;

    int amount; // z.B. Prozent bei IncreaseVolume; bei Absolute unbenutzt
};

// Eine Filterkette ist einfach eine Liste von Schritten,
// die nacheinander auf ein Sample angewendet werden.
using FilterChain = std::vector<FilterStep>;

// Bildet den Absolutwert eines Samples relativ zur Mittellinie.
unsigned char toAbsoluteSample(unsigned char sample);
// Macht ein Sample um "percent" Prozent lauter.
unsigned char increaseVolume(unsigned char sample, int percent);
// Wendet genau EINEN Filter-Schritt auf ein Sample an.
unsigned char applyFilterStep(unsigned char sample, const FilterStep& step);
// Wendet die GANZE Filterkette (alle Schritte nacheinander) auf ein Sample an.
unsigned char applyFilterChain(const FilterChain& filter_chain, unsigned char sample);
