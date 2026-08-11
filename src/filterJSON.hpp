/*
Deklariert die Funktionen und Datentypen zum Laden von Filter-JSON. Hier steht die API, die andere Teile des Programms benutzen, um die Konfiguration zu bekommen.
*/

#pragma once

#include <array>
#include <string>

#include "filters.hpp"

// Für jeden der 4 Kanäle gibt es genau eine Filterkette.
// Also: ein festes Array mit 4 Filterketten.
using ChannelFilterChains = std::array<FilterChain, 4>;

// Liest die JSON-Datei am angegebenen Pfad und gibt für jeden Kanal
// die passende Filterkette zurück.
ChannelFilterChains loadFilterChains(const std::string& config_path);
