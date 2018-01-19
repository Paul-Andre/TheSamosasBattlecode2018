#include "MapInfo.hpp"
#include <bc.h>

using namespace std;
using namespace bc;

MapInfo::MapInfo(const PlanetMap &map)
    : width((int)map.get_width()),
      height((int)map.get_height()),
      planet(map.get_planet()),
      location(width, vector<MapLocation *>(height)),
      karbonite(width, vector<unsigned>(height)),
      passable_terrain(width, vector<bool>(height)),
      can_sense(width, vector<bool>(height)),
      has_unit(width, vector<bool>(height)) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      auto ml = new MapLocation(planet, i, j);

      karbonite[i][j] = map.get_initial_karbonite_at(*ml);
      passable_terrain[i][j] = map.is_passable_terrain_at(*ml);
      location[i][j] = ml;
    }
  }
}

void MapInfo::update(const GameController &gc) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      auto ml = location[i][j];
      auto is_sensible = gc.can_sense_location(*ml);
      can_sense[i][j] = is_sensible;

      if (is_sensible) {
        karbonite[i][j] = gc.get_karbonite_at(*ml);
        has_unit[i][j] = gc.has_unit_at_location(*ml);
      }
    }
  }
}
