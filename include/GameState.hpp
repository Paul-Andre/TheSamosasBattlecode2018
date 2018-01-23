#pragma once

#include "MapInfo.hpp"
#include "UnitList.hpp"

using namespace bc;
using namespace std;

struct GameState {
  const Team MY_TEAM;
  const Team ENEMY_TEAM;
  const Planet PLANET;

  GameController& gc;
  uint32_t round;
  unsigned karbonite;

  MapInfo map_info;

  UnitList my_units;
  UnitList enemy_units;

  GameState(GameController& gc);

  void update();

  inline bool has_unit_at(int x, int y) const {
    return my_units.is_occupied[x][y] || enemy_units.is_occupied[x][y];
  }

  inline bool has_unit_at(const MapLocation& loc) const {
    return has_unit_at(loc.get_x(), loc.get_y());
  }

  bool is_surrounded(const MapLocation& loc) const;
  bool is_surrounding_enemy(const MapLocation& loc) const;

  void move(unsigned id, Direction dir);
  void load(unsigned structure_id, unsigned robot_id);
  unsigned unload(unsigned structure_id, Direction dir);
  void launch(unsigned rocket_id, const MapLocation& loc);
  void attack(unsigned id, unsigned target_id);

  void harvest(unsigned id, Direction dir);
  unsigned replicate(unsigned id, Direction dir);
  unsigned blueprint(unsigned id, UnitType unit_type, Direction direction);
};
