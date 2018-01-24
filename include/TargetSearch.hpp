#pragma once

#include <algorithm>
#include <limits>
#include <unordered_set>
#include <vector>

#include "GameState.hpp"
#include "PairwiseDistances.hpp"
#include "bc.hpp"

using namespace bc;
using namespace std;

struct Target {
  uint16_t distance;
  uint32_t id;
  uint8_t x;
  uint8_t y;
};

vector<Target> find_targets(GameState &game_state,
                            unordered_set<unsigned> units,
                            vector<MapLocation> target_locations,
                            const PairwiseDistances &distances) {
  vector<Target> targets;
  for (const auto unit_id : units) {
    const auto unit_loc = game_state.my_units.by_id[unit_id].second;
    const auto unit_x = unit_loc.get_x();
    const auto unit_y = unit_loc.get_y();

    for (const auto &target_loc : target_locations) {
      const auto target_x = target_loc.get_x();
      const auto target_y = target_loc.get_y();

      const auto distance =
          distances.get_distance(unit_x, unit_y, target_x, target_y);

      Target curr_target{distance, unit_id, static_cast<uint8_t>(target_x),
                         static_cast<uint8_t>(target_y)};
      targets.push_back(curr_target);
    }
  }

  // Sort by distance.
  sort(targets.begin(), targets.end(), [](const auto &a, const auto &b) {
    return !(a.distance >= b.distance);
  });

  return targets;
}
