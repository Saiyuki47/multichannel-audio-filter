
/*
Überwacht und steuert Worker/Prozesse: startet sie, prüft sie und reagiert bei Fehlern. Der Supervisor ist die Steuerlogik für die Laufzeitorganisation.

Grober Ablauf (Datenfluss):
  Datei --> Splitter --> [pipes] --> Worker (filtern) --> [result_pipes] --> Reassembler --> final.raw

Der Supervisor selbst startet all diese Prozesse mit fork() und wartet am Ende,
bis sie fertig sind.

Wichtig bei Pipes: Jeder Prozess soll nur die Enden offen halten, die er
wirklich benutzt. Alle anderen Enden muss er schließen – sonst merkt der
lesende Prozess nie, dass "nichts mehr kommt" (EOF). Deshalb gibt es unten die
vielen close...Ends()-Hilfsfunktionen.
*/
#include "supervisor.hpp"

#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>

#include "filterJSON.hpp"
#include "pipe_setup.hpp"
#include "splitter.hpp"
#include "streamReassemble.hpp"
#include "worker.hpp"

namespace
{
// Aufräumen im Worker-Prozess für Kanal "worker_index":
// Dieser Worker liest nur aus pipes[worker_index] und schreibt nur in
// result_pipes[worker_index]. Alles andere schließt er.
void closeWorkerProcessEnds(int pipes[CHANNELS][2], int result_pipes[CHANNELS][2], int worker_index)
{
    for(int j = 0; j < CHANNELS; ++j)
    {
        // In die Eingabe-Pipes schreibt nur der Splitter -> Worker schließt alle Schreib-Enden.
        close(pipes[j][1]);

        // Lesen darf der Worker nur aus SEINEM Kanal; die anderen Lese-Enden schließen.
        if(j != worker_index)
            close(pipes[j][0]);

        // Aus den Ergebnis-Pipes liest nur der Reassembler -> Worker schließt alle Lese-Enden.
        close(result_pipes[j][0]);

        // Schreiben darf der Worker nur in SEINEN Kanal; die anderen Schreib-Enden schließen.
        if(j != worker_index)
            close(result_pipes[j][1]);
    }
}

// Der Splitter benutzt die Ergebnis-Pipes gar nicht -> beide Enden schließen.
void closeSplitterEnds(int result_pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        close(result_pipes[i][0]);
        close(result_pipes[i][1]);
    }
}

// Der Reassembler liest nur aus den Ergebnis-Pipes (result_pipes[i][0]).
// Die Eingabe-Pipes braucht er nicht, und die Schreib-Enden der Ergebnis-Pipes auch nicht.
void closeReassemblerEnds(int pipes[CHANNELS][2], int result_pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
        close(result_pipes[i][1]);
    }
}

// Der Supervisor selbst leitet keine Daten weiter. Er hält keine Pipe-Enden
// offen, die eigentlich Splitter/Worker/Reassembler gehören – sonst käme nie ein EOF.
void closeSupervisorEnds(int pipes[CHANNELS][2], int result_pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
        close(result_pipes[i][1]);
    }
}
} // namespace

int runAudioSystem()
{
    int pipes[CHANNELS][2];        // Splitter -> Worker
    int result_pipes[CHANNELS][2]; // Worker -> Reassembler

    // Filterketten aus der JSON laden (pro Kanal eine Kette).
    ChannelFilterChains channel_filters = loadFilterChains("Filter/filters.json");
    const std::string final_output_path = "final.raw";

    std::cout << "Supervisor gestartet\n";

    // Beide Sätze Pipes anlegen. Klappt es nicht, sauber abbrechen.
    if(!createPipeSet(pipes))
        return 1;

    if(!createPipeSet(result_pipes))
    {
        closePipeSet(pipes);
        return 1;
    }

    pid_t worker_pid[CHANNELS]; // hier merken wir uns die Prozess-IDs der Worker

    // Für jeden Kanal einen Worker-Prozess starten.
    for(int i = 0; i < CHANNELS; ++i)
    {
        pid_t pid = fork(); // ab hier gibt es Eltern- und Kindprozess

        if(pid == 0)
        {
            // ---- Kindprozess: das ist der Worker ----
            closeWorkerProcessEnds(pipes, result_pipes, i);

            worker(
                i + 1,                 // Kanalnummer (1-basiert, nur zur Anzeige)
                pipes[i][0],           // liest rohe Samples von hier
                result_pipes[i][1],    // schreibt gefilterte Samples hierhin
                channel_filters[i]     // die Filterkette dieses Kanals
            );

            return 0; // Worker fertig -> Kindprozess beenden
        }

        // ---- Elternprozess: PID des Kindes merken ----
        worker_pid[i] = pid;
    }

    // Splitter-Prozess starten (liest die Datei und verteilt die Samples).
    pid_t splitter_pid = fork();

    if(splitter_pid == 0)
    {
        closeSplitterEnds(result_pipes);
        splitter(pipes);
        return 0;
    }

    // Reassembler-Prozess starten (setzt die gefilterten Samples zusammen).
    pid_t reassembler_pid = fork();

    if(reassembler_pid == 0)
    {
        closeReassemblerEnds(pipes, result_pipes);

        assembleOutputFromPipes(
            result_pipes,
            final_output_path
        );

        return 0;
    }

    // ---- Ab hier nur noch der Supervisor (Elternprozess) ----
    // Er selbst braucht keine Pipe-Enden mehr und schließt sie,
    // damit die Kindprozesse ihr EOF bekommen.
    closeSupervisorEnds(pipes, result_pipes);

    std::cout << "Supervisor wartet\n";

    // Auf den Splitter warten (er ist als Erstes fertig, wenn die Datei durch ist).
    waitpid(splitter_pid, nullptr, 0);
    std::cout << "Splitter beendet\n";

    // Dann auf alle Worker warten.
    for(int i = 0; i < CHANNELS; ++i)
        waitpid(worker_pid[i], nullptr, 0);

    // Zum Schluss auf den Reassembler warten.
    waitpid(reassembler_pid, nullptr, 0);

    std::cout << "System beendet\n";
    return 0;
}
