# 4-Kanal-RAW-Audio mit Filterketten pro Kanal
# Build: `make watcher`
# Run:   `make run`
CXX      ?= g++
CXXFLAGS ?= -O2 -Wall -Wextra -std=c++17

WATCHER_SRCS := src/main.cpp src/filters.cpp src/filterJSON.cpp src/worker.cpp src/streamReassemble.cpp src/splitter.cpp src/pipe_setup.cpp src/supervisor.cpp src/gpio.cpp
RAWTESTER_SRCS := rawtester.cpp src/filters.cpp src/filterJSON.cpp
INPUT_RAW    := input/audio_data_team1.raw
SOURCE_RAW   := input/audio_data_team1.raw

all: watcher

rawtester: $(RAWTESTER_SRCS)
	$(CXX) $(CXXFLAGS) -I. -o $@ $(RAWTESTER_SRCS)

watcher: $(WATCHER_SRCS)
	$(CXX) $(CXXFLAGS) -pthread -o $@ $(WATCHER_SRCS) -lgpiod

$(INPUT_RAW): $(SOURCE_RAW)
	cp $< $@

.PHONY: all run clean watcher run-watcher rawtester
run: watcher $(INPUT_RAW)
	./watcher
run-watcher: run

clean:
	rm -f watcher final.raw $(INPUT_RAW)
