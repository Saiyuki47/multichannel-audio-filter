## Das komplette Projekt ist eine Zusammenarbeit von :
### - David Salz 
### - Leon Döhrer 
### - Hendrik Voß 


# Multichannel Audio Filter

[![CI](https://github.com/Saiyuki47/multichannel-audio-filter/actions/workflows/ci.yml/badge.svg)](https://github.com/Saiyuki47/multichannel-audio-filter/actions/workflows/ci.yml)

Ein C++-Tool zur Verarbeitung von 4-Kanal-RAW-Audio mit einer eigenen Filterkette pro Kanal. Das System liest eine RAW-Audiodatei, teilt sie in vier Kanäle auf, wendet pro Kanal die in einer JSON-Datei konfigurierten Filter an und setzt das Ergebnis wieder zu einer Ausgabedatei zusammen.

## Architektur

Das Programm arbeitet mit mehreren Prozessen, die über Pipes kommunizieren:

- **Supervisor** (`supervisor.cpp`) — startet und überwacht alle Prozesse (Splitter, Worker, Reassembler) und organisiert die Pipe-Verbindungen.
- **Splitter** (`splitter.cpp`) — verteilt die eingelesenen Samples auf die vier Kanal-Pipes.
- **Worker** (`worker.cpp`) — ein Prozess pro Kanal; wendet die konfigurierte Filterkette auf jedes Sample an.
- **Reassembler** (`reassemble.cpp`, `streamReassemble.cpp`) — fügt die gefilterten Kanäle wieder zur finalen Ausgabe (`final.raw`) zusammen.
- **Filter** (`filters.cpp`, `filterJSON.cpp`) — die Filter-Implementierungen (`increaseVolume`, `absolute`) sowie das Laden der Filterketten aus JSON.
- **GPIO** (`gpio.cpp`) — Hardware-Anbindung über `libgpiod` (z. B. auf einem Raspberry Pi).

## Filter konfigurieren

Die Filterketten werden in `Filter/filters.json` pro Kanal definiert, z. B.:

```json
{
  "channels": {
    "1": [ { "type": "increaseVolume", "amount": 25 } ],
    "3": [ { "type": "increaseVolume", "amount": 25 }, { "type": "absolute" } ]
  }
}
```

Verfügbare Filtertypen:

- `increaseVolume` mit `amount` (Lautstärkeanhebung in Prozent)
- `absolute` (Betrag relativ zur Mittellinie)

## Build

Voraussetzungen: `g++` mit C++17 und `libgpiod` (`-lgpiod`).

```sh
make watcher     # baut das Hauptprogramm 'watcher'
make rawtester   # baut das Test-Tool 'rawtester'
```

## Ausführen

```sh
make run         # baut 'watcher' und führt es aus
```

Die Eingabe wird aus `input/audio_data_team1.raw` gelesen, die Ausgabe nach `final.raw` geschrieben.

```sh
make clean       # entfernt Build-Artefakte und Ausgaben
```

## Verzeichnisstruktur

```
src/            C++-Quellcode (Supervisor, Worker, Splitter, Filter, GPIO, ...)
Filter/         Filter-Konfiguration (filters.json)
input/          RAW-Audio-Eingabedaten
rawtester.cpp   Eigenständiges Test-Tool für die Filter
Makefile        Build-Definitionen
```

## Continuous Integration (GitHub Actions)

Bei jedem Push und Pull Request laufen automatisch drei Prüfungen
(`.github/workflows/ci.yml`):

- **Build & Warnungen** — baut `watcher` und `rawtester` unter Ubuntu (inkl. `libgpiod`) mit strengen Compiler-Warnungen (`-Wall -Wextra -Wpedantic`).
- **cppcheck** — statische Analyse, die mögliche Bugs findet, ohne das Programm auszuführen. Bekannte, bewusst akzeptierte Hinweise stehen in `.cppcheck-suppressions`.
- **clang-format** — prüft den einheitlichen Code-Stil nach `.clang-format`.

Lokal lassen sich dieselben Prüfungen ausführen:

```sh
make watcher CXXFLAGS="-O2 -Wall -Wextra -Wpedantic -std=c++17"
cppcheck --enable=warning,style,performance,portability --std=c++17 \
  --inline-suppr --suppressions-list=.cppcheck-suppressions -I. rawtester.cpp src/
clang-format --dry-run --Werror rawtester.cpp src/*.cpp src/*.hpp
clang-format -i rawtester.cpp src/*.cpp src/*.hpp   # Formatierung anwenden
```

## Hinweis

Das kompilierte `watcher`-Binary war für ARM aarch64 (z. B. Raspberry Pi) gebaut und ist nicht Teil des Repositories — es wird lokal per `make` erzeugt.
