#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "PairwiseDistances.hpp"

#include "bc.hpp"
#include "constants.hpp"

// TODO: make some kind of GameState object that I can pass easily
Direction silly_pathfinding(GameController &gc, const MapLocation &start,
                            const MapLocation &goal,
                            const PairwiseDistances &pd) {
  int unit_x = start.get_x();
  int unit_y = start.get_y();

  int x = goal.get_x();
  int y = goal.get_y();

  // Used to sort by distance and then by random
  vector<pair<pair<unsigned short, int>, int>> v;
  for (int k = 0; k < constants::N_DIRECTIONS; k++) {
    int xx = unit_x + constants::DX[k];
    int yy = unit_y + constants::DY[k];
    v.push_back(make_pair(make_pair(pd.get_distance(x, y, xx, yy), rand()), k));
  }

  auto current = v[8].first.first;
  sort(v.begin(), v.end());

  for (int i = 0; i < constants::N_DIRECTIONS; i++) {
    auto next = v[i].first.first;
    Direction dir = Direction(v[i].second);
    MapLocation new_location = start.add(dir);
    if (gc.can_sense_location(new_location) && gc.is_occupiable(new_location) &&
        next <= current) {
      return dir;
    }
  }

  return Center;
}
