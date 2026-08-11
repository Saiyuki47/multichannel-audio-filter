/*
Lädt Filterinformationen aus einer JSON-Datei und wandelt sie in interne Datenstrukturen um. Diese Datei kümmert sich um das Einlesen, Parsen und Validieren der JSON-Einträge, damit die Filter später angewendet werden können.
*/


#include "filterJSON.hpp"

#include <cctype>
#include <fstream>
#include <iterator>

namespace
{
    std::string readFileToString(const std::string& path)
    {
        std::ifstream input(path, std::ios::binary);

        if(!input)
            return {};

        return std::string(
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        );
    }

    std::string trim(const std::string& text)
    {
        std::size_t start = 0;
        std::size_t end = text.size();

        while(start < end && std::isspace(static_cast<unsigned char>(text[start])))
            ++start;

        while(end > start && std::isspace(static_cast<unsigned char>(text[end - 1])))
            --end;

        return text.substr(start, end - start);
    }

    std::size_t findMatchingBracket(const std::string& text, std::size_t open_pos, char open_bracket, char close_bracket)
    {
        int depth = 0;

        for(std::size_t i = open_pos; i < text.size(); ++i)
        {
            if(text[i] == open_bracket)
                ++depth;
            else if(text[i] == close_bracket)
            {
                --depth;

                if(depth == 0)
                    return i;
            }
        }

        return std::string::npos;
    }

    std::string extractStringField(const std::string& object_text, const std::string& field_name)
    {
        std::string needle = std::string("\"") + field_name + "\"";
        std::size_t field_pos = object_text.find(needle);

        if(field_pos == std::string::npos)
            return {};

        std::size_t colon_pos = object_text.find(':', field_pos + needle.size());

        if(colon_pos == std::string::npos)
            return {};

        std::size_t quote_start = object_text.find('"', colon_pos + 1);

        if(quote_start == std::string::npos)
            return {};

        std::size_t quote_end = object_text.find('"', quote_start + 1);

        if(quote_end == std::string::npos)
            return {};

        return object_text.substr(quote_start + 1, quote_end - quote_start - 1);
    }

    int extractIntField(const std::string& object_text, const std::string& field_name, int fallback)
    {
        std::string needle = std::string("\"") + field_name + "\"";
        std::size_t field_pos = object_text.find(needle);

        if(field_pos == std::string::npos)
            return fallback;

        std::size_t colon_pos = object_text.find(':', field_pos + needle.size());

        if(colon_pos == std::string::npos)
            return fallback;

        std::size_t value_start = colon_pos + 1;

        while(value_start < object_text.size() && std::isspace(static_cast<unsigned char>(object_text[value_start])))
            ++value_start;

        std::size_t value_end = value_start;

        while(value_end < object_text.size() && std::isdigit(static_cast<unsigned char>(object_text[value_end])))
            ++value_end;

        if(value_end == value_start)
            return fallback;

        return std::stoi(object_text.substr(value_start, value_end - value_start));
    }

    FilterChain parseFilterChainForChannel(const std::string& json_text, int channel)
    {
        FilterChain chain;
        std::string channel_key = std::string("\"") + std::to_string(channel) + "\"";
        std::size_t channel_pos = json_text.find(channel_key);

        if(channel_pos == std::string::npos)
            return chain;

        std::size_t array_start = json_text.find('[', channel_pos);

        if(array_start == std::string::npos)
            return chain;

        std::size_t array_end = findMatchingBracket(json_text, array_start, '[', ']');

        if(array_end == std::string::npos)
            return chain;

        std::string array_text = json_text.substr(array_start + 1, array_end - array_start - 1);
        std::size_t object_pos = 0;

        while((object_pos = array_text.find('{', object_pos)) != std::string::npos)
        {
            std::size_t object_end = findMatchingBracket(array_text, object_pos, '{', '}');

            if(object_end == std::string::npos)
                break;

            std::string object_text = array_text.substr(object_pos, object_end - object_pos + 1);
            std::string type = trim(extractStringField(object_text, "type"));

            if(type == "increaseVolume")
            {
                int amount = extractIntField(object_text, "amount", 25);
                chain.push_back({FilterStep::Type::IncreaseVolume, amount});
            }
            else if(type == "absolute")
            {
                chain.push_back({FilterStep::Type::Absolute, 0});
            }

            object_pos = object_end + 1;
        }

        return chain;
    }
}

ChannelFilterChains loadFilterChains(const std::string& config_path)
{
    ChannelFilterChains chains;
    std::string json_text = readFileToString(config_path);

    if(json_text.empty())
    {
        chains[0] = {{FilterStep::Type::IncreaseVolume, 25}};
        chains[1] = {{FilterStep::Type::IncreaseVolume, 25}};
        chains[2] = {{FilterStep::Type::IncreaseVolume, 25}, {FilterStep::Type::Absolute, 0}};
        chains[3] = {{FilterStep::Type::IncreaseVolume, 25}, {FilterStep::Type::Absolute, 0}};
        return chains;
    }

    for(int channel = 1; channel <= 4; ++channel)
    {
        chains[channel - 1] = parseFilterChainForChannel(json_text, channel);

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
