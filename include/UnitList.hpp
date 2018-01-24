#pragma once

#include <array>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "bc.hpp"
#include "constants.hpp"

using namespace bc;
using namespace std;

struct UnitList {
  const Team TEAM;
  const Planet PLANET;
  const unsigned WIDTH;
  const unsigned HEIGHT;

  array<unordered_set<unsigned>, constants::N_UNIT_TYPES> by_type;
  unordered_map<unsigned, pair<UnitType, MapLocation>> by_id;
  vector<vector<unsigned>> by_location;
  vector<vector<bool>> is_occupied;
  unordered_set<unsigned> all;

  unordered_map<unsigned, MapLocation> initial_workers;

  UnitList(GameController& gc, const Team& team);

  void add(unsigned id, UnitType unit_type, MapLocation loc);

  void remove(unsigned id);

  void move(unsigned id, Direction dir);

  void update(GameController& gc);
};
