
/*
Überwacht und steuert Worker/Prozesse: startet sie, prüft sie und reagiert bei Fehlern. Der Supervisor ist die Steuerlogik für die Laufzeitorganisation.
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
void closeWorkerProcessEnds(int pipes[CHANNELS][2], int result_pipes[CHANNELS][2], int worker_index)
{
    for(int j = 0; j < CHANNELS; ++j)
    {
        close(pipes[j][1]);

        if(j != worker_index)
            close(pipes[j][0]);

        close(result_pipes[j][0]);

        if(j != worker_index)
            close(result_pipes[j][1]);
    }
}

void closeSplitterEnds(int result_pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        close(result_pipes[i][0]);
        close(result_pipes[i][1]);
    }
}

void closeReassemblerEnds(int pipes[CHANNELS][2], int result_pipes[CHANNELS][2])
{
    for(int i = 0; i < CHANNELS; ++i)
    {
        close(pipes[i][0]);
        close(pipes[i][1]);
        close(result_pipes[i][1]);
    }
}

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
    int pipes[CHANNELS][2];
    int result_pipes[CHANNELS][2];
    ChannelFilterChains channel_filters = loadFilterChains("Filter/filters.json");
    const std::string final_output_path = "final.raw";

    std::cout << "Supervisor gestartet\n";

    if(!createPipeSet(pipes))
        return 1;

    if(!createPipeSet(result_pipes))
    {
        closePipeSet(pipes);
        return 1;
    }

    pid_t worker_pid[CHANNELS];

    for(int i = 0; i < CHANNELS; ++i)
    {
        pid_t pid = fork();

        if(pid == 0)
        {
            closeWorkerProcessEnds(pipes, result_pipes, i);

            worker(
                i + 1,
                pipes[i][0],
                result_pipes[i][1],
                channel_filters[i]
            );

            return 0;
        }

        worker_pid[i] = pid;
    }

    pid_t splitter_pid = fork();

    if(splitter_pid == 0)
    {
        closeSplitterEnds(result_pipes);
        splitter(pipes);
        return 0;
    }

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

    closeSupervisorEnds(pipes, result_pipes);

    std::cout << "Supervisor wartet\n";

    waitpid(splitter_pid, nullptr, 0);
    std::cout << "Splitter beendet\n";

    for(int i = 0; i < CHANNELS; ++i)
        waitpid(worker_pid[i], nullptr, 0);

    waitpid(reassembler_pid, nullptr, 0);

    std::cout << "System beendet\n";
    return 0;
}