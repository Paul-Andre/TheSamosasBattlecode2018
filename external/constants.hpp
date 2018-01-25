#pragma once

#include <limits>
#include <unordered_set>
#include <vector>

#include "bc.hpp"

// FIXME: Copied from PairwiseDistances.hpp.
std::vector<std::pair<int, int>> make_kernel(int min_distance_squared,
                                             int max_distance_squared);

using namespace std;

namespace constants {
// Map.
constexpr static int MAX_MAP_SIZE = 50;
constexpr static int MIN_MAP_SIZE = 20;

// Rounds.
constexpr static int FLOOD_ROUND = 750;
constexpr static int N_ROUNDS = 1000;

// Units.
constexpr static int N_UNIT_TYPES = 8;
constexpr static int N_ROBOT_TYPES = 5;
const static unordered_set<unsigned> ROBOT_TYPES = {Worker, Knight, Ranger,
                                                    Mage, Healer};
const static unordered_set<unsigned> STRUCTURE_TYPES = {Factory, Rocket};

// Directions.
constexpr static int N_DIRECTIONS = 9;
constexpr static int N_DIRECTIONS_WITHOUT_CENTER = N_DIRECTIONS - 1;
constexpr static int DX[N_DIRECTIONS] = {0, 1, 1, 1, 0, -1, -1, -1, 0};
constexpr static int DY[N_DIRECTIONS] = {1, 1, 0, -1, -1, -1, 0, 1, 0};
constexpr static bc::Direction DIRECTIONS[N_DIRECTIONS] = {
    North,     Northeast, East,      Southeast, South,
    Southwest, West,      Northwest, Center};

// Costs.
const static unsigned BLUEPRINT_COST[N_UNIT_TYPES] = {
    numeric_limits<unsigned>::max(),  // Worker
    numeric_limits<unsigned>::max(),  // Knight
    numeric_limits<unsigned>::max(),  // Ranger
    numeric_limits<unsigned>::max(),  // Mage
    numeric_limits<unsigned>::max(),  // Healer
    bc::unit_type_get_blueprint_cost(Factory),
    bc::unit_type_get_blueprint_cost(Rocket),
};
const static unsigned FACTORY_COST[N_UNIT_TYPES] = {
    bc::unit_type_get_factory_cost(Worker),
    bc::unit_type_get_factory_cost(Knight),
    bc::unit_type_get_factory_cost(Ranger),
    bc::unit_type_get_factory_cost(Mage),
    bc::unit_type_get_factory_cost(Healer),
    numeric_limits<unsigned>::max(),  // Factory
    numeric_limits<unsigned>::max(),  // Rocket
};
const static unsigned REPLICATION_COST = bc::unit_type_get_replicate_cost();

// Values.
const static unsigned UNIT_VALUES[N_UNIT_TYPES] = {
    bc::unit_type_get_value(Worker), bc::unit_type_get_value(Knight),
    bc::unit_type_get_value(Ranger), bc::unit_type_get_value(Mage),
    bc::unit_type_get_value(Healer), bc::unit_type_get_value(Factory),
    bc::unit_type_get_value(Rocket),
};

// Research.
constexpr static int N_RESEARCH_LEVELS = 4;
const static unsigned RESEARCH_COST[N_UNIT_TYPES][N_RESEARCH_LEVELS] = {
    {
        cost_of(Worker, 1),
        cost_of(Worker, 2),
        cost_of(Worker, 3),
        cost_of(Worker, 4),
    },
    {
        cost_of(Knight, 1),
        cost_of(Knight, 2),
        cost_of(Knight, 3),
        numeric_limits<unsigned>::max(),
    },
    {
        cost_of(Ranger, 1),
        cost_of(Ranger, 2),
        cost_of(Ranger, 3),
        numeric_limits<unsigned>::max(),
    },
    {
        cost_of(Mage, 1),
        cost_of(Mage, 2),
        cost_of(Mage, 3),
        cost_of(Mage, 4),
    },
    {
        cost_of(Healer, 1),
        cost_of(Healer, 2),
        cost_of(Healer, 3),
        numeric_limits<unsigned>::max(),
    },
    {
        // Can't research factories.
        numeric_limits<unsigned>::max(),
        numeric_limits<unsigned>::max(),
        numeric_limits<unsigned>::max(),
        numeric_limits<unsigned>::max(),
    },
    {
        cost_of(Rocket, 1),
        cost_of(Rocket, 2),
        cost_of(Rocket, 3),
        numeric_limits<unsigned>::max(),
    },
};

// Pairwise distances kernels.
// XXX: magic number from the specs.
const static vector<pair<int, int>> POINT_KERNEL = {{0, 0}};
const static vector<pair<int, int>> KERNEL[N_UNIT_TYPES] = {
    POINT_KERNEL,         // Worker
    POINT_KERNEL,         // Knight
    make_kernel(10, 50),  // Ranger
    make_kernel(-1, 30),  // Mage
    make_kernel(-1, 30),  // Healer
    POINT_KERNEL,         // Factory
    POINT_KERNEL,         // Rocket
};

// Attack ranges.
const static unsigned ATTACK_RANGE[N_UNIT_TYPES] = {
    0,   // Worker
    2,   // Knight
    50,  // Ranger
    30,  // Mage
    30,  // Healer
    0,   // Factory
    0,   // Rocket
};

}  // namespace constants
