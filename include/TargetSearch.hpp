#pragma once

#include <limits>
#include <unordered_set>
#include <vector>

#include "GameState.hpp"
#include "PairwiseDistances.hpp"
#include "bc.hpp"

using namespace bc;
using namespace std;

#pragma pack(1)
struct Target {
  uint16_t distance;
  uint32_t id;
  uint8_t x;
  uint8_t y;
};
#pragma pack()

union TargetUnion {
  uint64_t score;
  Target target;
};

vector<Target> find_targets(GameState &game_state,
                            unordered_set<unsigned> units,
                            vector<MapLocation> target_locations,
                            PairwiseDistances &distances) {
  vector<uint64_t> targets_as_uint;
  for (const auto unit_id : units) {
    const auto unit_loc = game_state.my_units.by_id[unit_id].second;
    const auto unit_x = unit_loc.get_x();
    const auto unit_y = unit_loc.get_y();

    Target optimal_target{std::numeric_limits<uint16_t>::max(), unit_id, 0, 0};
    for (const auto &target_loc : target_locations) {
      const auto target_x = target_loc.get_x();
      const auto target_y = target_loc.get_y();

      const auto distance =
          distances.get_distance(unit_x, unit_y, target_x, target_y);
      if (distance < optimal_target.distance) {
        optimal_target.distance = distance;
        optimal_target.x = target_x;
        optimal_target.y = target_y;
      }
    }

    TargetUnion target_union;
    target_union.target = optimal_target;
    targets_as_uint.push_back(target_union.score);
  }

  // Sort by score.
  sort(targets_as_uint.begin(), targets_as_uint.end());

  vector<Target> targets;
  for (const auto &target_as_uint : targets_as_uint) {
    targets.push_back(TargetUnion{target_as_uint}.target);
  }

  return targets;
}