/*
Setzt die gefilterten Samples LIVE aus den vier Kanal-Pipes wieder zusammen und
schreibt sie in die Ausgabedatei. Reihenfolge: Kanal0, Kanal1, Kanal2, Kanal3,
Kanal0, ... – genau umgekehrt zum Splitter.

Zwei Besonderheiten:
1) Die Pipes werden "non-blocking" gemacht, damit ein leerer Kanal das Lesen
   nicht blockiert. Kommt gerade nichts, warten wir mit poll() auf neue Daten.
2) Pro geschriebenem Sample wird ein GPIO-Impuls für den Kanal ausgegeben.
*/

#include "streamReassemble.hpp"

#include <array>
#include <deque>
#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

#include "gpio.hpp"

void assembleOutputFromPipes(int pipes[4][2], const std::string& final_output_path)
{
    constexpr int channel_count = 4;

    // Ausgabedatei öffnen (binär, vorher leeren).
    std::ofstream output(
        final_output_path,
        std::ios::binary | std::ios::trunc);


    if(!output)
    {
        std::cerr
            << "Kann Ausgabedatei nicht oeffnen: "
            << final_output_path
            << std::endl;

        return;
    }

    // GPIOs vorbereiten. Klappt es nicht, läuft das Programm trotzdem weiter
    // (nur ohne die Hardware-Impulse).
    if(!initGpios())
    {
        std::cerr << "Kann GPIOs nicht initialisieren" << std::endl;
    }


    // Pro Kanal ein Zwischenpuffer (Warteschlange) für schon gelesene Bytes.
    std::array<std::deque<unsigned char>, channel_count> buffers;
    // Merker pro Kanal: ist die Pipe schon zu (kommt nichts mehr)?
    std::array<bool, channel_count> channel_closed{};
    // Struktur für poll(): damit fragen wir "gibt es neue Daten?" ab.
    std::array<pollfd, channel_count> poll_fds{};


    for(int i = 0; i < channel_count; ++i)
    {
        // poll() soll auf das Lese-Ende jeder Pipe schauen.
        poll_fds[i].fd = pipes[i][0];
        poll_fds[i].events = POLLIN | POLLHUP | POLLERR; // Daten da / geschlossen / Fehler
        poll_fds[i].revents = 0;


        // Die Pipe auf "non-blocking" umstellen: read() wartet dann nicht,
        // sondern meldet sofort, wenn gerade nichts da ist.
        int flags = fcntl(pipes[i][0], F_GETFL, 0);


        if(flags >= 0)
            fcntl(pipes[i][0], F_SETFL, flags | O_NONBLOCK);
    }


    // Kleine Hilfe: Sind ALLE Kanäle zu UND alle Puffer leer? Dann sind wir fertig.
    auto allChannelsDone = [&channel_closed, &buffers]()
    {
        for(int i = 0; i < channel_count; ++i)
        {
            if(!channel_closed[i] || !buffers[i].empty())
                return false;
        }


        return true;
    };


    // Hilfe: aus allen offenen Kanälen so viel lesen wie gerade da ist,
    // und in die jeweiligen Puffer legen.
    auto readAvailableData = [&]()
    {
        bool made_progress = false;


        for(int i = 0; i < channel_count; ++i)
        {
            if(channel_closed[i]) // dieser Kanal ist schon fertig
                continue;


            while(true)
            {
                unsigned char sample;
                ssize_t bytes_read = read(poll_fds[i].fd, &sample, 1);


                if(bytes_read > 0)
                {
                    // Ein Byte gelesen -> in den Puffer und weiterlesen.
                    buffers[i].push_back(sample);
                    made_progress = true;
                    continue;
                }


                if(bytes_read == 0)
                {
                    // 0 = Pipe geschlossen (EOF): Kanal ist fertig.
                    channel_closed[i] = true;
                    break;
                }


                // Ab hier ist bytes_read < 0, also ein "Fehler".
                if(bytes_read < 0 && errno == EINTR)
                    continue; // nur unterbrochen -> nochmal versuchen


                if(bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    break; // gerade nichts da (non-blocking) -> aufhören


                break; // anderer Fehler -> aufhören
            }
        }


        return made_progress;
    };


    int next_channel = 0; // welcher Kanal ist als Nächstes "dran"


    // Hauptschleife: immer versuchen, das nächste Sample in der richtigen
    // Reihenfolge herauszuschreiben.
    while(true)
    {
        bool emitted_sample = false;   // haben wir in dieser Runde etwas geschrieben?
        bool any_pending_data = false; // liegt irgendwo noch etwas im Puffer?


        for(int i = 0; i < channel_count; ++i)
        {
            if(!buffers[i].empty())
            {
                any_pending_data = true;
                break;
            }
        }


        // Ab dem "dran"-Kanal reihum schauen, wer das nächste Sample liefert.
        for(int offset = 0; offset < channel_count; ++offset)
        {
            int channel = (next_channel + offset) % channel_count;


            if(buffers[channel].empty())
            {
                // Kanal leer, aber schon geschlossen? Dann überspringen und
                // beim nächsten Kanal weitermachen.
                if(channel_closed[channel])
                    continue;


                // Kanal leer, aber noch offen -> wir müssen auf ihn warten,
                // um die Reihenfolge zu halten. Also hier abbrechen.
                break;
            }


            // Vorderstes Byte dieses Kanals in die Ausgabe schreiben.
            output.write(
                reinterpret_cast<const char*>(&buffers[channel].front()),
                1);

            output.flush();                               // sofort rausschreiben (Live-Ausgabe)
            pulseGpio(static_cast<std::size_t>(channel)); // Hardware-Impuls für den Kanal
            buffers[channel].pop_front();                 // Byte ist verarbeitet -> entfernen
            emitted_sample = true;
            next_channel = (channel + 1) % channel_count; // nächster Kanal ist dran
            break;
        }


        // Haben wir etwas geschrieben? Dann direkt die nächste Runde.
        if(emitted_sample)
        {
            continue;
        }


        // Nichts geschrieben und alles fertig? Dann raus.
        if(allChannelsDone())
        {
            break;
        }


        // Nichts geschrieben und auch nichts im Puffer -> auf neue Daten warten.
        if(!any_pending_data)
        {
            // poll() blockiert (-1 = ohne Timeout), bis an einer Pipe etwas passiert.
            int poll_result = poll(poll_fds.data(), poll_fds.size(), -1);


            if(poll_result < 0)
            {
                if(errno == EINTR)
                    continue; // nur unterbrochen -> nochmal


                std::cerr
                    << "Kann Pipes nicht abfragen"
                    << std::endl;


                break;
            }


            // Es gibt neue Daten -> in die Puffer einlesen.
            readAvailableData();
        }
    }


    // Aufräumen: alle Lese-Enden schließen und GPIOs freigeben.
    for(int i = 0; i < channel_count; ++i)
    {
        close(pipes[i][0]);
    }

    closeGpios();
}
