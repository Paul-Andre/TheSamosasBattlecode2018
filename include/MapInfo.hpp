#pragma once

#include <vector>
#include "bc.hpp"

using namespace std;
using namespace bc;

struct MapInfo {
  const int width;
  const int height;
  const Planet planet;

  // Convention: [x][y].
  vector<vector<unsigned>> karbonite;
  vector<vector<bool>> passable_terrain;
  vector<vector<bool>> can_sense;

  MapInfo(const PlanetMap &map);

  void update(const GameController &gc);

  inline MapLocation get_location(int x, int y) const {
    return MapLocation(planet, x, y);
  }

  MapLocation get_random_passable_location() const;

  inline bool is_valid_location(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
  }
};
