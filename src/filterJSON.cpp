/*
Lädt Filterinformationen aus einer JSON-Datei und wandelt sie in interne Datenstrukturen um. Diese Datei kümmert sich um das Einlesen, Parsen und Validieren der JSON-Einträge, damit die Filter später angewendet werden können.

Hinweis: Hier wird die JSON absichtlich "von Hand" (mit find/substr) zerlegt,
damit keine externe JSON-Bibliothek nötig ist. Es ist also kein vollständiger
JSON-Parser, sondern reicht genau für unser einfaches Format.
*/


#include "filterJSON.hpp"

#include <cctype>
#include <fstream>
#include <iterator>

namespace
{
    // Liest die komplette Datei in einen einzigen String ein.
    // Wenn die Datei nicht geöffnet werden kann, kommt ein leerer String zurück.
    std::string readFileToString(const std::string& path)
    {
        std::ifstream input(path, std::ios::binary);

        if(!input)
            return {};

        // Von Anfang bis Ende alle Zeichen in einen String kopieren.
        return std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }

    // Entfernt Leerzeichen/Whitespace am Anfang und Ende eines Textes.
    std::string trim(const std::string& text)
    {
        std::size_t start = 0;
        std::size_t end = text.size();

        // Von vorne so lange weiterrücken, wie Leerzeichen kommen.
        while(start < end && std::isspace(static_cast<unsigned char>(text[start])))
            ++start;

        // Von hinten genauso.
        while(end > start && std::isspace(static_cast<unsigned char>(text[end - 1])))
            --end;

        return text.substr(start, end - start);
    }

    // Sucht zu einer öffnenden Klammer die passende schließende Klammer.
    // Trick: mitzählen, wie tief man verschachtelt ist. Bei Tiefe 0 ist Schluss.
    std::size_t findMatchingBracket(const std::string& text, std::size_t open_pos, char open_bracket, char close_bracket)
    {
        int depth = 0;

        for(std::size_t i = open_pos; i < text.size(); ++i)
        {
            if(text[i] == open_bracket)
                ++depth; // eine Ebene tiefer
            else if(text[i] == close_bracket)
            {
                --depth; // eine Ebene raus

                // Wenn wir wieder ganz draußen sind, ist das die passende Klammer.
                if(depth == 0)
                    return i;
            }
        }

        // Keine passende Klammer gefunden.
        return std::string::npos;
    }

    // Holt aus einem JSON-Objekt-Text den Wert eines Text-Feldes, z.B.
    // aus {"type": "absolute"} bei field_name="type" das Wort "absolute".
    std::string extractStringField(const std::string& object_text, const std::string& field_name)
    {
        // Wir suchen z.B. den Text  "type"  (mit Anführungszeichen).
        std::string needle = std::string("\"") + field_name + "\"";
        std::size_t field_pos = object_text.find(needle);

        if(field_pos == std::string::npos)
            return {}; // Feld gibt es nicht

        // Nach dem Feldnamen kommt der Doppelpunkt.
        std::size_t colon_pos = object_text.find(':', field_pos + needle.size());

        if(colon_pos == std::string::npos)
            return {};

        // Der Wert steht zwischen dem nächsten Paar Anführungszeichen.
        std::size_t quote_start = object_text.find('"', colon_pos + 1);

        if(quote_start == std::string::npos)
            return {};

        std::size_t quote_end = object_text.find('"', quote_start + 1);

        if(quote_end == std::string::npos)
            return {};

        // Nur den Text zwischen den Anführungszeichen zurückgeben.
        return object_text.substr(quote_start + 1, quote_end - quote_start - 1);
    }

    // Wie extractStringField, aber für eine Zahl, z.B. "amount": 25.
    // Wenn nichts gefunden wird, kommt der "fallback"-Wert zurück.
    int extractIntField(const std::string& object_text, const std::string& field_name, int fallback)
    {
        std::string needle = std::string("\"") + field_name + "\"";
        std::size_t field_pos = object_text.find(needle);

        if(field_pos == std::string::npos)
            return fallback;

        std::size_t colon_pos = object_text.find(':', field_pos + needle.size());

        if(colon_pos == std::string::npos)
            return fallback;

        // Hinter dem Doppelpunkt zuerst evtl. Leerzeichen überspringen.
        std::size_t value_start = colon_pos + 1;

        while(value_start < object_text.size() && std::isspace(static_cast<unsigned char>(object_text[value_start])))
            ++value_start;

        // Dann so weit lesen, wie Ziffern kommen.
        std::size_t value_end = value_start;

        while(value_end < object_text.size() && std::isdigit(static_cast<unsigned char>(object_text[value_end])))
            ++value_end;

        // Keine einzige Ziffer gefunden -> fallback.
        if(value_end == value_start)
            return fallback;

        // Den gefundenen Ziffern-Text in eine Zahl umwandeln.
        return std::stoi(object_text.substr(value_start, value_end - value_start));
    }

    // Liest aus dem gesamten JSON-Text die Filterkette für EINEN Kanal.
    // Kanal ist hier 1..4 (so wie in der JSON geschrieben).
    FilterChain parseFilterChainForChannel(const std::string& json_text, int channel)
    {
        FilterChain chain;

        // Wir suchen den Schlüssel des Kanals, z.B.  "3" .
        std::string channel_key = std::string("\"") + std::to_string(channel) + "\"";
        std::size_t channel_pos = json_text.find(channel_key);

        if(channel_pos == std::string::npos)
            return chain; // Kanal steht nicht in der Datei

        // Nach dem Kanal folgt die eckige Klammer [ ... ] mit den Schritten.
        std::size_t array_start = json_text.find('[', channel_pos);

        if(array_start == std::string::npos)
            return chain;

        // Passende schließende Klammer ] finden.
        std::size_t array_end = findMatchingBracket(json_text, array_start, '[', ']');

        if(array_end == std::string::npos)
            return chain;

        // Nur den Text INNERHALB der eckigen Klammern betrachten.
        std::string array_text = json_text.substr(array_start + 1, array_end - array_start - 1);
        std::size_t object_pos = 0;

        // Jetzt jedes einzelne Objekt { ... } in der Liste durchgehen.
        while((object_pos = array_text.find('{', object_pos)) != std::string::npos)
        {
            std::size_t object_end = findMatchingBracket(array_text, object_pos, '{', '}');

            if(object_end == std::string::npos)
                break;

            // Den Text dieses einen Objekts herausschneiden.
            std::string object_text = array_text.substr(object_pos, object_end - object_pos + 1);

            // Welcher Filter-Typ ist es?
            std::string type = trim(extractStringField(object_text, "type"));

            if(type == "increaseVolume")
            {
                // Lautstärke-Filter: "amount" auslesen (Standard 25%).
                int amount = extractIntField(object_text, "amount", 25);
                chain.push_back({FilterStep::Type::IncreaseVolume, amount});
            }
            else if(type == "absolute")
            {
                // Absolutwert-Filter: braucht keinen Zahlenwert.
                chain.push_back({FilterStep::Type::Absolute, 0});
            }

            // Weiter hinter dem gerade gelesenen Objekt suchen.
            object_pos = object_end + 1;
        }

        return chain;
    }
}

// Öffentliche Funktion: lädt die Filterketten für alle 4 Kanäle.
ChannelFilterChains loadFilterChains(const std::string& config_path)
{
    ChannelFilterChains chains;
    std::string json_text = readFileToString(config_path);

    // Konnte die Datei nicht gelesen werden? Dann nehmen wir sinnvolle
    // Standard-Filter, damit das Programm trotzdem läuft.
    if(json_text.empty())
    {
        chains[0] = {{FilterStep::Type::IncreaseVolume, 25}};
        chains[1] = {{FilterStep::Type::IncreaseVolume, 25}};
        chains[2] = {{FilterStep::Type::IncreaseVolume, 25}, {FilterStep::Type::Absolute, 0}};
        chains[3] = {{FilterStep::Type::IncreaseVolume, 25}, {FilterStep::Type::Absolute, 0}};
        return chains;
    }

    // Für jeden Kanal 1..4 die Kette aus der JSON holen.
    for(int channel = 1; channel <= 4; ++channel)
    {
        // Achtung: intern zählen wir ab 0, in der JSON ab 1.
        chains[channel - 1] = parseFilterChainForChannel(json_text, channel);

        // Kam für den Kanal nichts heraus? Dann wieder einen Standard setzen.
        if(chains[channel - 1].empty())
        {
            if(channel <= 2)
                chains[channel - 1] = {{FilterStep::Type::IncreaseVolume, 25}};
            else
                chains[channel - 1] = {{FilterStep::Type::IncreaseVolume, 25}, {FilterStep::Type::Absolute, 0}};
        }
    }

    return chains;
}
