/*
Enthält die eigentlichen Filter-Operationen (z.B. Lautstärke erhöhen, Absolutwert). Diese Funktionen nehmen ein Sample, verarbeiten es Schritt für Schritt und liefern das veränderte Byte zurück.
*/

#include "filters.hpp"
#include <cmath>
#include <cstdlib>



unsigned char applyFilterStep(unsigned char sample, const FilterStep& step)
{
    if(step.type == FilterStep::Type::IncreaseVolume)
        return increaseVolume(sample, step.amount);

    return toAbsoluteSample(sample);
}

unsigned char applyFilterChain(const FilterChain& filter_chain, unsigned char sample)
{
    unsigned char processed_sample = sample;

    for(const FilterStep& step : filter_chain)
    {
        processed_sample = applyFilterStep(processed_sample, step);
    }

    return processed_sample;
}


unsigned char toAbsoluteSample(unsigned char sample)
{
    double centered_sample = static_cast<double>(sample) - SAMPLE_MIDPOINT;

    double absolute_value = std::abs(centered_sample);

    long result = std::lround(absolute_value + SAMPLE_MIDPOINT);

    if(result < 0)
        result = 0;

    if(result > 255)
        result = 255;

    return static_cast<unsigned char>(result);
}

unsigned char increaseVolume(unsigned char sample, int percent)
{
    double centered_sample = static_cast<double>(sample) - SAMPLE_MIDPOINT;

    double amplified =
        centered_sample * (100.0 + static_cast<double>(percent)) / 100.0;


    long result = std::lround(amplified + SAMPLE_MIDPOINT);

    if(result < 0)
        result = 0;

    if(result > 255)
        result = 255;


    return static_cast<unsigned char>(result);
}