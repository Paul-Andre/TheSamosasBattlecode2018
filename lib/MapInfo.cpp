#include "MapInfo.hpp"
#include <bc.h>

using namespace std;
using namespace bc;

MapInfo::MapInfo(const PlanetMap &map)
    : width((int)map.get_width()),
      height((int)map.get_height()),
      planet(map.get_planet()),
      karbonite(width, vector<unsigned>(height)),
      passable_terrain(width, vector<bool>(height)),
      location(width, vector<bc_MapLocation *>(height)) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      MapLocation mp{planet, i, j};

      karbonite[i][j] = map.get_initial_karbonite_at(mp);
      passable_terrain[i][j] = map.is_passable_terrain_at(mp);

      location[i][j] = new_bc_MapLocation(planet, i, j);
    }
  }
}

void MapInfo::update_karbonite(const GameController &gc) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      bc_MapLocation *mp = location[i][j];
      auto m_gc = gc.get_bc();
      if (bc_GameController_can_sense_location(m_gc, mp)) {
        karbonite[i][j] = bc_GameController_karbonite_at(m_gc, mp);
      }
    }
  }
}
