#pragma once

#include <algorithm>
#include <utility>
#include <vector>

#include "GameState.hpp"
#include "PairwiseDistances.hpp"

#include "bc.hpp"
#include "constants.hpp"

// TODO: make some kind of GameState object that I can pass easily
Direction silly_pathfinding(GameState &game_state, const MapLocation &start,
                            const MapLocation &goal,
                            const PairwiseDistances &pd) {
  int unit_x = start.get_x();
  int unit_y = start.get_y();

  int x = goal.get_x();
  int y = goal.get_y();

  // Used to sort by distance and then by random
  vector<pair<pair<unsigned short, int>, int>> v;
  for (int k = 0; k < constants::N_DIRECTIONS; k++) {
    const auto xx = unit_x + constants::DX[k];
    const auto yy = unit_y + constants::DY[k];
    v.push_back(make_pair(make_pair(pd.get_distance(x, y, xx, yy), rand()), k));
  }

  const auto current = v[Center].first.first;
  sort(v.begin(), v.end());

  for (int i = 0; i < constants::N_DIRECTIONS; i++) {
    const auto next = v[i].first.first;
    if (next > current) continue;

    const auto k = v[i].second;
    const auto xx = unit_x + constants::DX[k];
    const auto yy = unit_y + constants::DY[k];

    if (!game_state.map_info.can_sense[xx][yy]) continue;
    if (!game_state.map_info.passable_terrain[xx][yy]) continue;
    if (game_state.has_unit_at(xx, yy)) continue;

    return static_cast<Direction>(k);
  }

  return Center;
}
