/*
Deklariert die Funktionen und Datentypen zum Laden von Filter-JSON. Hier steht die API, die andere Teile des Programms benutzen, um die Konfiguration zu bekommen.
*/

#pragma once

#include <array>
#include <string>

#include "filters.hpp"

using ChannelFilterChains = std::array<FilterChain, 4>;

ChannelFilterChains loadFilterChains(const std::string& config_path);
