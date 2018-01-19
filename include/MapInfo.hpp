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
  vector<vector<bool>> sensible;
  vector<vector<bool>> passable_terrain;
  vector<vector<MapLocation *>> location;

  MapInfo(const PlanetMap &map);

  void update(const GameController &gc);

  inline bool is_valid_location(int x, int y) const {
    return x >= 0 && x < width && y >= 0 && y < height;
  }
};
