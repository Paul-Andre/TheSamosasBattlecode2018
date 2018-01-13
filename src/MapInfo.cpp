#include "MapInfo.hpp"

using namespace std;
using namespace bc;

MapInfo::MapInfo(const PlanetMap &map)
    : width((int)map.get_width()),
      height((int)map.get_height()),
      planet(map.get_planet()),
      karbonite(width, vector<unsigned>(height)),
      passable_terrain(width, vector<bool>(height)) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      MapLocation mp{map.get_planet(), i, j};

      karbonite[i][j] = map.get_initial_karbonite_at(mp);
      passable_terrain[i][j] = map.is_passable_terrain_at(mp);
    }
  }
}

void MapInfo::update_karbonite(const GameController &gc) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      MapLocation mp{planet, i, j};

      if (gc.can_sense_location(mp)) {
        karbonite[i][j] = gc.get_karbonite_at(mp);
      }
    }
  }
}
