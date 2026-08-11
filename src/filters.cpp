/*
Enthält die eigentlichen Filter-Operationen (z.B. Lautstärke erhöhen, Absolutwert). Diese Funktionen nehmen ein Sample, verarbeiten es Schritt für Schritt und liefern das veränderte Byte zurück.
*/

#include "filters.hpp"
#include <cmath>
#include <cstdlib>



// Schaut nach, welche Art von Filter der Schritt ist,
// und ruft die passende Funktion auf.
unsigned char applyFilterStep(unsigned char sample, const FilterStep& step)
{
    // Ist es "lauter machen"? Dann verstärken.
    if(step.type == FilterStep::Type::IncreaseVolume)
        return increaseVolume(sample, step.amount);

    // Sonst ist es der Absolutwert-Filter.
    return toAbsoluteSample(sample);
}

// Geht die komplette Filterkette durch und wendet jeden Schritt
// nacheinander auf das Sample an. Das Ergebnis des einen Schritts
// ist die Eingabe für den nächsten.
unsigned char applyFilterChain(const FilterChain& filter_chain, unsigned char sample)
{
    unsigned char processed_sample = sample;

    // Für jeden Filter-Schritt in der Kette ...
    for(const FilterStep& step : filter_chain)
    {
        // ... das (schon bearbeitete) Sample weiter bearbeiten.
        processed_sample = applyFilterStep(processed_sample, step);
    }

    return processed_sample;
}


// Bildet den "Betrag" der Auslenkung um die Mittellinie.
// Werte, die weit unter der Mitte liegen, werden dadurch nach oben
// gespiegelt – das Sample zeigt also immer eine Auslenkung nach oben.
unsigned char toAbsoluteSample(unsigned char sample)
{
    // Erst um die Mittellinie zentrieren: jetzt kann der Wert negativ sein.
    double centered_sample = static_cast<double>(sample) - SAMPLE_MIDPOINT;

    // Vorzeichen wegwerfen -> immer positiv.
    double absolute_value = std::abs(centered_sample);

    // Wieder zurück auf die Byte-Skala verschieben und runden.
    long result = std::lround(absolute_value + SAMPLE_MIDPOINT);

    // Sicherheitshalber im gültigen Byte-Bereich 0..255 halten.
    if(result < 0)
        result = 0;

    if(result > 255)
        result = 255;

    return static_cast<unsigned char>(result);
}

// Macht ein Sample lauter, indem die Auslenkung um die Mittellinie
// um "percent" Prozent vergrößert wird.
unsigned char increaseVolume(unsigned char sample, int percent)
{
    // Zuerst zur Mittellinie zentrieren (Auslenkung kann negativ sein).
    double centered_sample = static_cast<double>(sample) - SAMPLE_MIDPOINT;

    // Auslenkung skalieren: percent=25 bedeutet Faktor 1.25 (25% lauter).
    double amplified =
        centered_sample * (100.0 + static_cast<double>(percent)) / 100.0;


    // Zurück auf die Byte-Skala schieben und runden.
    long result = std::lround(amplified + SAMPLE_MIDPOINT);

    // Übersteuern verhindern: im Bereich 0..255 bleiben (clipping).
    if(result < 0)
        result = 0;

    if(result > 255)
        result = 255;


    return static_cast<unsigned char>(result);
}
