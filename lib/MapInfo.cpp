#include "MapInfo.hpp"
#include <bc.h>

using namespace std;
using namespace bc;

MapInfo::MapInfo(const PlanetMap &map)
    : width((int)map.get_width()),
      height((int)map.get_height()),
      planet(map.get_planet()),
      karbonite(width, vector<unsigned>(height)),
      sensible(width, vector<bool>(height)),
      passable_terrain(width, vector<bool>(height)),
      location(width, vector<MapLocation *>(height)) {
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
      sensible[i][j] = is_sensible;
      if (is_sensible) {
        karbonite[i][j] = gc.get_karbonite_at(*ml);
      }
    }
  }
}
