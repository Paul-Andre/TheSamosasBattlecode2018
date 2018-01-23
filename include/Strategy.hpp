#pragma once

#include <iostream>

#include <limits>
#include <unordered_map>
#include <unordered_set>

#include "GameState.hpp"
#include "TargetSearch.hpp"
#include "constants.hpp"
#include "silly_pathfinding.hpp"

using namespace std;

class Strategy {
 public:
  virtual void run(GameState &game_state, unordered_set<unsigned> units) = 0;
};

class RobotStrategy : public Strategy {
 protected:
  void move(GameState &game_state, unsigned unit_id, const MapLocation &goal,
            PairwiseDistances &pd) {
    const auto &loc = game_state.my_units.by_id[unit_id].second;
    const auto dir = silly_pathfinding(game_state.gc, loc, goal, pd);
    if (game_state.gc.can_move(unit_id, dir) &&
        game_state.gc.is_move_ready(unit_id)) {
      game_state.move(unit_id, dir);
    }
  }

  void move(GameState &game_state, unsigned unit_id) {
    auto seed = rand();
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      auto dir = (Direction)((i + seed) % 8);
      if (game_state.gc.can_move(unit_id, dir) &&
          game_state.gc.is_move_ready(unit_id)) {
        game_state.move(unit_id, dir);
        return;
      }
    }
  }

  bool maybe_board_rocket(GameState &game_state, unsigned unit_id) {
    const auto &loc = game_state.my_units.by_id[unit_id].second;
    const auto robot_x = loc.get_x();
    const auto robot_y = loc.get_y();

    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto probe_x = robot_x + constants::DX[i];
      const auto probe_y = robot_y + constants::DY[i];

      if (!game_state.map_info.is_valid_location(probe_x, probe_y)) continue;
      if (!game_state.my_units.is_occupied[probe_x][probe_y]) continue;

      const auto other_id = game_state.my_units.by_location[probe_x][probe_y];
      const auto other_type = game_state.my_units.by_id[other_id].first;
      if (other_type != Rocket) continue;

      // It's a rocket!
      if (!game_state.gc.can_load(other_id, unit_id)) continue;

      game_state.load(other_id, unit_id);
      return true;
    }
    return false;
  }
};

class WorkerStrategy : public RobotStrategy {
 protected:
  void maybe_move_and_replicate(GameState &game_state, unsigned worker_id,
                                const MapLocation &goal,
                                const PairwiseDistances &pd,
                                bool should_replicate) {
    auto loc = game_state.my_units.by_id[worker_id].second;
    auto dir = silly_pathfinding(game_state.gc, loc, goal, pd);
    if (game_state.gc.can_move(worker_id, dir) &&
        game_state.gc.is_move_ready(worker_id)) {
      game_state.move(worker_id, dir);
      loc = loc.add(dir);
    }

    if (should_replicate) {
      dir = silly_pathfinding(game_state.gc, loc, goal, pd);
      if (game_state.gc.can_replicate(worker_id, dir)) {
        const auto replicated_id = game_state.replicate(worker_id, dir);
        return maybe_move_and_replicate(game_state, replicated_id, goal, pd,
                                        false);
      } else {
        const auto seed = rand();
        for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
          auto dir =
              (Direction)((i + seed) % constants::N_DIRECTIONS_WITHOUT_CENTER);
          if (game_state.gc.can_replicate(worker_id, dir)) {
            const auto replicated_id = game_state.replicate(worker_id, dir);
            return maybe_move_and_replicate(game_state, replicated_id, goal, pd,
                                            false);
          }
        }
      }
    }

    maybe_harvest(game_state, worker_id);
    maybe_build_or_repair(game_state, worker_id);
  }

  void maybe_move_and_replicate_randomly(GameState &game_state,
                                         unsigned worker_id,
                                         bool should_replicate) {
    auto seed = rand();
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto dir =
          (Direction)((i + seed) % constants::N_DIRECTIONS_WITHOUT_CENTER);
      if (game_state.gc.can_move(worker_id, dir) &&
          game_state.gc.is_move_ready(worker_id)) {
        game_state.move(worker_id, dir);
        break;
      }
    }

    if (should_replicate) {
      seed = rand();
      for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
        const auto dir =
            (Direction)((i + seed) % constants::N_DIRECTIONS_WITHOUT_CENTER);
        if (game_state.gc.can_replicate(worker_id, dir)) {
          const auto replicated_id = game_state.replicate(worker_id, dir);
          return maybe_move_and_replicate_randomly(game_state, replicated_id,
                                                   false);
        }
      }
    }

    maybe_harvest(game_state, worker_id);
    maybe_build_or_repair(game_state, worker_id);
  }

  bool maybe_harvest(GameState &game_state, unsigned worker_id) {
    const auto &loc = game_state.my_units.by_id[worker_id].second;
    const auto worker_x = loc.get_x();
    const auto worker_y = loc.get_y();

    auto best_dir = Center;
    unsigned max_karbonite = 0;
    for (int i = 0; i < constants::N_DIRECTIONS; i++) {
      const auto dir = (Direction)i;
      const auto harvest_x = worker_x + constants::DX[i];
      const auto harvest_y = worker_y + constants::DY[i];

      if (game_state.gc.can_harvest(worker_id, dir)) {
        const auto karbonite =
            game_state.map_info.karbonite[harvest_x][harvest_y];
        if (karbonite > max_karbonite) {
          max_karbonite = karbonite;
          best_dir = dir;
        }
      }
    }

    if (game_state.gc.can_harvest(worker_id, best_dir)) {
      game_state.harvest(worker_id, best_dir);
      return true;
    }

    return false;
  }

  bool maybe_build_or_repair(GameState &game_state, unsigned worker_id) {
    const auto &loc = game_state.my_units.by_id[worker_id].second;
    const auto worker_x = loc.get_x();
    const auto worker_y = loc.get_y();

    // Try build.
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto build_x = worker_x + constants::DX[i];
      const auto build_y = worker_y + constants::DY[i];

      if (!game_state.map_info.is_valid_location(build_x, build_y)) continue;
      if (!game_state.my_units.is_occupied[build_x][build_y]) continue;
      const auto unit_id = game_state.my_units.by_location[build_x][build_y];
      const auto unit_type = game_state.my_units.by_id[unit_id].first;

      if (unit_type != Factory && unit_type != Rocket) continue;

      const auto unit = game_state.gc.get_unit(unit_id);
      if (unit.structure_is_built()) continue;
      if (!game_state.gc.can_build(worker_id, unit_id)) continue;

      game_state.gc.build(worker_id, unit_id);

      return true;
    }

    // Try repair.
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto build_x = worker_x + constants::DX[i];
      const auto build_y = worker_y + constants::DY[i];

      if (!game_state.map_info.is_valid_location(build_x, build_y)) continue;
      if (!game_state.my_units.is_occupied[build_x][build_y]) continue;
      const auto unit_id = game_state.my_units.by_location[build_x][build_y];
      const auto unit_type = game_state.my_units.by_id[unit_id].first;

      if (unit_type != Factory && unit_type != Rocket) continue;

      const auto unit = game_state.gc.get_unit(unit_id);
      if (!unit.structure_is_built()) continue;
      if (unit.get_health() == unit.get_max_health()) continue;

      if (!game_state.gc.can_repair(worker_id, unit_id)) continue;

      game_state.gc.repair(worker_id, unit_id);

      return true;
    }

    return false;
  }

  bool maybe_blueprint(GameState &game_state, unsigned worker_id,
                       UnitType unit_type) {
    const auto &loc = game_state.my_units.by_id[worker_id].second;
    const auto worker_x = loc.get_x();
    const auto worker_y = loc.get_y();

    const auto MAX_OBSTRUCTIONS = 3;
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto probe_x = worker_x + constants::DX[i];
      const auto probe_y = worker_y + constants::DY[i];

      if (count_obstructions(game_state, probe_x, probe_y) >= MAX_OBSTRUCTIONS)
        continue;

      const auto dir = (Direction)i;
      if (game_state.gc.can_blueprint(worker_id, unit_type, dir)) {
        game_state.blueprint(worker_id, unit_type, dir);
        return true;
      }
    }

    return false;
  }

  unsigned count_obstructions(GameState &game_state, unsigned x, unsigned y) {
    unsigned obstructions = 0;
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto probe_x = x + constants::DX[i];
      const auto probe_y = y + constants::DY[i];

      if (!game_state.map_info.is_valid_location(probe_x, probe_y)) continue;
      if (game_state.enemy_units.is_occupied[probe_x][probe_y]) obstructions++;
      if (!game_state.map_info.passable_terrain[probe_x][probe_y])
        obstructions++;
    }
    return obstructions;
  }

  bool is_safe_location(GameState &game_state, unsigned x, unsigned y,
                        int radius) {
    // TODO: Improve based on enemy unit types and their attack ranges.
    for (int i = -radius; i <= radius; i++) {
      for (int j = -radius; j <= radius; j++) {
        const auto probe_x = x + i;
        const auto probe_y = y + j;

        if (!game_state.map_info.is_valid_location(probe_x, probe_y)) continue;
        if (game_state.enemy_units.is_occupied[probe_x][probe_y]) return false;
      }
    }
    return true;
  }
};

class WorkerRushStrategy : public WorkerStrategy {
  const PairwiseDistances distances;

 public:
  WorkerRushStrategy(const PairwiseDistances &distances)
      : distances(distances) {}

  void run(GameState &game_state, unordered_set<unsigned> workers) {
    vector<MapLocation> target_locations;

    unordered_map<uint16_t, unsigned> n_max_targetting;
    for (const auto &unit : game_state.enemy_units.by_id) {
      const auto loc = unit.second.second;
      target_locations.push_back(loc);

      const auto x = loc.get_x();
      const auto y = loc.get_y();
      const uint16_t hash = (x << 8) + y;
      n_max_targetting[hash] = constants::N_DIRECTIONS_WITHOUT_CENTER -
                               count_obstructions(game_state, x, y);
    }

    const auto targets =
        find_targets(game_state, workers, target_locations, distances);

    unordered_map<uint16_t, unsigned> n_targetting;
    unordered_set<unsigned> targetting;

    auto should_replicate = true;
    for (const auto &target : targets) {
      if (target.distance == numeric_limits<uint16_t>::max()) continue;

      const uint16_t hash = (target.x << 8) + target.y;
      if (n_targetting[hash] >= n_max_targetting[hash]) continue;
      if (targetting.count(target.id)) continue;

      const auto loc = game_state.map_info.location[target.x][target.y];
      if (game_state.is_surrounded(*loc) && target.distance > 1) continue;

      n_targetting[hash]++;
      targetting.insert(target.id);

      const auto goal = game_state.map_info.location[target.x][target.y];
      maybe_move_and_replicate(game_state, target.id, *goal, distances,
                               should_replicate);
    }

    for (const auto worker_id : workers) {
      if (targetting.count(worker_id)) continue;
      maybe_move_and_replicate_randomly(game_state, worker_id,
                                        should_replicate);
    }
  }
};