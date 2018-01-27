#include "MapInfo.hpp"

using namespace std;
using namespace bc;

MapInfo::MapInfo(const PlanetMap &map)
    : width((int)map.get_width()),
      height((int)map.get_height()),
      planet(map.get_planet()),
      karbonite(width, vector<float>(height)),
      passable_terrain(width, vector<bool>(height)),
      can_sense(width, vector<bool>(height)) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      const auto ml = get_location(i, j);
      karbonite[i][j] = map.get_initial_karbonite_at(ml);
      passable_terrain[i][j] = map.is_passable_terrain_at(ml);
    }
  }
}

MapLocation MapInfo::get_random_passable_location() const {
  while (true) {
    int x = rand() % width;
    int y = rand() % height;
    if (passable_terrain[x][y]) {
      return get_location(x, y);
    }
  }
}

void MapInfo::update(const GameController &gc) {
  for (int i = 0; i < width; i++) {
    for (int j = 0; j < height; j++) {
      const auto ml = get_location(i, j);
      const auto is_sensible = gc.can_sense_location(ml);
      can_sense[i][j] = is_sensible;

      if (is_sensible) {
        karbonite[i][j] = gc.get_karbonite_at(ml);
      } else {
        // Depreciate unseen karbonite.
        karbonite[i][j] *= 0.99;
        if (karbonite[i][j] < 1) karbonite[i][j] = 0;
      }
    }
  }
}
