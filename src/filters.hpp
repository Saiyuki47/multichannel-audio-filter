/*
Definiert die Filter-Schritte und die Signaturen der Filterfunktionen. Andere Module importieren hier die Filter-Funktionen und Datentypen.
*/
#pragma once

#include <vector>

constexpr double SAMPLE_MIDPOINT = 126.1;

struct FilterStep
{
    enum class Type
    {
        IncreaseVolume,
        Absolute
    } type;

    int amount;
};

using FilterChain = std::vector<FilterStep>;

unsigned char toAbsoluteSample(unsigned char sample);
unsigned char increaseVolume(unsigned char sample, int percent);
unsigned char applyFilterStep(unsigned char sample, const FilterStep& step);
unsigned char applyFilterChain(const FilterChain& filter_chain, unsigned char sample);
