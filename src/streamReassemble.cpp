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

    if(!initGpios())
    {
        std::cerr << "Kann GPIOs nicht initialisieren" << std::endl;
    }


    std::array<std::deque<unsigned char>, channel_count> buffers;
    std::array<bool, channel_count> channel_closed{};
    std::array<pollfd, channel_count> poll_fds{};


    for(int i = 0; i < channel_count; ++i)
    {
        poll_fds[i].fd = pipes[i][0];
        poll_fds[i].events = POLLIN | POLLHUP | POLLERR;
        poll_fds[i].revents = 0;


        int flags = fcntl(pipes[i][0], F_GETFL, 0);


        if(flags >= 0)
            fcntl(pipes[i][0], F_SETFL, flags | O_NONBLOCK);
    }


    auto allChannelsDone = [&channel_closed, &buffers]()
    {
        for(int i = 0; i < channel_count; ++i)
        {
            if(!channel_closed[i] || !buffers[i].empty())
                return false;
        }


        return true;
    };


    auto readAvailableData = [&]()
    {
        bool made_progress = false;


        for(int i = 0; i < channel_count; ++i)
        {
            if(channel_closed[i])
                continue;


            while(true)
            {
                unsigned char sample;
                ssize_t bytes_read = read(poll_fds[i].fd, &sample, 1);


                if(bytes_read > 0)
                {
                    buffers[i].push_back(sample);
                    made_progress = true;
                    continue;
                }


                if(bytes_read == 0)
                {
                    channel_closed[i] = true;
                    break;
                }


                if(bytes_read < 0 && errno == EINTR)
                    continue;


                if(bytes_read < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
                    break;


                break;
            }
        }


        return made_progress;
    };


    int next_channel = 0;


    while(true)
    {
        bool emitted_sample = false;
        bool any_pending_data = false;


        for(int i = 0; i < channel_count; ++i)
        {
            if(!buffers[i].empty())
            {
                any_pending_data = true;
                break;
            }
        }


        for(int offset = 0; offset < channel_count; ++offset)
        {
            int channel = (next_channel + offset) % channel_count;


            if(buffers[channel].empty())
            {
                if(channel_closed[channel])
                    continue;


                break;
            }


            output.write(
                reinterpret_cast<const char*>(&buffers[channel].front()),
                1
            );

            output.flush();
            pulseGpio(static_cast<std::size_t>(channel));
            buffers[channel].pop_front();
            emitted_sample = true;
            next_channel = (channel + 1) % channel_count;
            break;
        }


        if(emitted_sample)
        {
            continue;
        }


        if(allChannelsDone())
        {
            break;
        }


        if(!any_pending_data)
        {
            int poll_result = poll(poll_fds.data(), poll_fds.size(), -1);


            if(poll_result < 0)
            {
                if(errno == EINTR)
                    continue;


                std::cerr
                    << "Kann Pipes nicht abfragen"
                    << std::endl;


                break;
            }


            readAvailableData();
        }
    }


    for(int i = 0; i < channel_count; ++i)
    {
        close(pipes[i][0]);
    }

    closeGpios();
}
