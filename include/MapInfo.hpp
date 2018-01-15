#pragma once

#include <vector>
#include "bc.hpp"

using namespace std;
using namespace bc;

/// Note that I use the 2d array convention [x][y]
struct MapInfo {
  int width;
  int height;
  Planet planet;
  vector<vector<unsigned>> karbonite;
  vector<vector<bool>> passable_terrain;
  vector<vector<bc_MapLocation *>> location;

  MapInfo(const PlanetMap &map);

  void update_karbonite(const GameController &gc);
};
