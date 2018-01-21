#pragma once

#include "MapInfo.hpp"
#include "UnitList.hpp"

using namespace bc;
using namespace std;

struct GameState {
  const Team TEAM;
  const Planet PLANET;

  uint32_t round;
  unsigned karbonite;

  MapInfo map_info;

  UnitList my_units;
  UnitList enemy_units;

  GameState(GameController& gc);

  void update(GameController& gc);
};