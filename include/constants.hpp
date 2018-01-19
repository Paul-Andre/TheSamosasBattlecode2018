#pragma once

#include "bc.hpp"

namespace constants {
// Map.
constexpr static int MAX_MAP_SIZE = 50;
constexpr static int MIN_MAP_SIZE = 20;

// Rounds.
constexpr static int FLOOD_ROUND = 750;
constexpr static int N_ROUNDS = 1000;

// Units.
constexpr static int N_UNIT_TYPES = 8;

// Directions.
constexpr static int N_DIRECTIONS = 9;
constexpr static int N_DIRECTIONS_WITHOUT_CENTER = N_DIRECTIONS - 1;
constexpr static int DX[N_DIRECTIONS] = {0, 1, 1, 1, 0, -1, -1, -1, 0};
constexpr static int DY[N_DIRECTIONS] = {1, 1, 0, -1, -1, -1, 0, 1, 0};
constexpr static bc::Direction DIRECTIONS[N_DIRECTIONS] = {
    North,     Northeast, East,      Southeast, South,
    Southwest, West,      Northwest, Center};
};  // namespace constants
