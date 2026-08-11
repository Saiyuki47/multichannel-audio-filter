/*
Nimmt aufgeteilte Daten-Chunks und setzt sie wieder zu vollständigen Samples zusammen. Verantwortlich für das korrekte Zusammenfügen von Segmenten in die Originalfolge.
*/


#include "reassemble.hpp"


#include <fstream>
#include <iostream>

void assembleOutput(const std::string channel_outputs[4], const std::string& final_output_path)
{
    std::ifstream inputs[4];
    std::ofstream output(
        final_output_path,
        std::ios::binary | std::ios::trunc
    );


    if(!output)
    {
        std::cerr
            << "Kann Ausgabedatei nicht oeffnen: "
            << final_output_path
            << std::endl;

        return;
    }


    for(int i=0;i<4;i++)
    {
        inputs[i].open(
            channel_outputs[i],
            std::ios::binary
        );


        if(!inputs[i])
        {
            std::cerr
                << "Kann Kanaldatei nicht oeffnen: "
                << channel_outputs[i]
                << std::endl;

            return;
        }
    }


    unsigned char sample;
    bool wrote_data = true;


    while(wrote_data)
    {
        wrote_data = false;


        for(int i=0;i<4;i++)
        {
            if(inputs[i].read(
                reinterpret_cast<char*>(&sample),
                1))
            {
                output.write(
                    reinterpret_cast<const char*>(&sample),
                    1
                );

                wrote_data = true;
            }
        }
    }
}
