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
  vector<vector<MapLocation *>> location;

  MapInfo(const PlanetMap &map);

  void update_karbonite(const GameController &gc);
};
