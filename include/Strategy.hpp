#pragma once

#include <iostream>

#include <limits>
#include <queue>
#include <unordered_map>
#include <unordered_set>

#include "GameState.hpp"
#include "TargetSearch.hpp"
#include "constants.hpp"
#include "silly_pathfinding.hpp"

using namespace std;

class Strategy {
 public:
  virtual ~Strategy() {}
  virtual bool run(GameState &game_state, unordered_set<unsigned> units) = 0;
};

class NullStrategy : public Strategy {
 public:
  bool run(GameState &game_state, unordered_set<unsigned> units) {
    return true;
  }
};

class RobotStrategy : public Strategy {
 protected:
  bool should_move_to_enemy = true;
  bool should_move_to_rockets = true;
  bool should_move_to_factories = false;

 public:
  void set_should_move_to_enemy(bool status) {
    should_move_to_rockets = status;
  }

  void set_should_move_to_rockets(bool status) {
    should_move_to_rockets = status;
  }

  void set_should_move_to_factories(bool status) {
    should_move_to_factories = status;
  }

 protected:
  void maybe_move(GameState &game_state, unsigned unit_id,
                  const MapLocation &goal, const PairwiseDistances &pd) {
    const auto loc = game_state.my_units.by_id[unit_id].second;
    const auto dir = silly_pathfinding(game_state, loc, goal, pd);
    if (game_state.gc.can_move(unit_id, dir) &&
        game_state.gc.is_move_ready(unit_id)) {
      game_state.move(unit_id, dir);
    }
  }

  void maybe_move_randomly(GameState &game_state, unsigned unit_id) {
    auto seed = rand();
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      auto dir = static_cast<Direction>((i + seed) % 8);
      if (game_state.gc.can_move(unit_id, dir) &&
          game_state.gc.is_move_ready(unit_id)) {
        game_state.move(unit_id, dir);
        return;
      }
    }
  }

  bool maybe_board_rocket(GameState &game_state, unsigned unit_id) {
    const auto loc = game_state.my_units.by_id[unit_id].second;
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
  bool should_replicate = true;
  bool should_move_to_enemy = false;
  bool should_move_to_karbonite = true;
  bool should_move_to_unbuilt_rockets = true;
  bool should_move_to_unbuilt_factories = true;

 public:
  void set_should_replicate(bool status) { should_replicate = status; }
  void set_should_move_to_unbuilt_factories(bool status) {
    should_move_to_unbuilt_factories = status;
  }
  void set_should_move_to_unbuilt_rockets(bool status) {
    should_move_to_unbuilt_rockets = status;
  }
  void set_should_move_to_karbonite(bool status) {
    should_move_to_karbonite = true;
  }

 protected:
  void maybe_move_and_replicate(GameState &game_state, unsigned worker_id,
                                const MapLocation &goal,
                                const PairwiseDistances &pd, bool should_move,
                                bool should_replicate) {
    auto loc = game_state.my_units.by_id[worker_id].second;
    if (should_move) {
      const auto dir = silly_pathfinding(game_state, loc, goal, pd);
      if (game_state.gc.can_move(worker_id, dir) &&
          game_state.gc.is_move_ready(worker_id)) {
        game_state.move(worker_id, dir);
        loc = loc.add(dir);
      }
    }

    if (should_replicate) {
      const auto dir = silly_pathfinding(game_state, loc, goal, pd);
      if (game_state.gc.can_replicate(worker_id, dir)) {
        const auto replicated_id = game_state.replicate(worker_id, dir);
        return maybe_move_and_replicate(game_state, replicated_id, goal, pd,
                                        true, false);
      } else {
        const auto seed = rand();
        for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
          const auto dir = static_cast<Direction>(
              (i + seed) % constants::N_DIRECTIONS_WITHOUT_CENTER);
          if (game_state.gc.can_replicate(worker_id, dir)) {
            const auto replicated_id = game_state.replicate(worker_id, dir);
            return maybe_move_and_replicate(game_state, replicated_id, goal, pd,
                                            true, false);
          }
        }
      }
    }

    if (maybe_build_or_repair(game_state, worker_id)) return;
    if (maybe_harvest(game_state, worker_id)) return;
  }

  void maybe_move_and_replicate_randomly(GameState &game_state,
                                         unsigned worker_id, bool should_move,
                                         bool should_replicate) {
    if (should_move) {
      const auto seed = rand();
      for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
        const auto dir = static_cast<Direction>(
            (i + seed) % constants::N_DIRECTIONS_WITHOUT_CENTER);
        if (game_state.gc.can_move(worker_id, dir) &&
            game_state.gc.is_move_ready(worker_id)) {
          game_state.move(worker_id, dir);
          break;
        }
      }
    }

    if (should_replicate) {
      const auto seed = rand();
      for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
        const auto dir = static_cast<Direction>(
            (i + seed) % constants::N_DIRECTIONS_WITHOUT_CENTER);
        if (game_state.gc.can_replicate(worker_id, dir)) {
          const auto replicated_id = game_state.replicate(worker_id, dir);
          return maybe_move_and_replicate_randomly(game_state, replicated_id,
                                                   true, false);
        }
      }
    }

    if (maybe_build_or_repair(game_state, worker_id)) return;
    if (maybe_harvest(game_state, worker_id)) return;
  }

  bool maybe_harvest(GameState &game_state, unsigned worker_id) {
    const auto loc = game_state.my_units.by_id[worker_id].second;
    const auto worker_x = loc.get_x();
    const auto worker_y = loc.get_y();

    auto best_dir = Center;
    auto can_harvest = false;
    unsigned max_karbonite = 0;
    for (int i = 0; i < constants::N_DIRECTIONS; i++) {
      const auto dir = static_cast<Direction>(i);
      const auto probe_x = worker_x + constants::DX[i];
      const auto probe_y = worker_y + constants::DY[i];
      if (!game_state.map_info.is_valid_location(probe_x, probe_y)) continue;

      if (game_state.gc.can_harvest(worker_id, dir)) {
        can_harvest = true;
        const auto karbonite = game_state.map_info.karbonite[probe_x][probe_y];
        if (karbonite > max_karbonite) {
          max_karbonite = karbonite;
          best_dir = dir;
        }
      }
    }

    if (can_harvest) {
      game_state.harvest(worker_id, best_dir);
      return true;
    }

    return false;
  }

  bool maybe_build_or_repair(GameState &game_state, unsigned worker_id) {
    const auto loc = game_state.my_units.by_id[worker_id].second;
    const auto worker_x = loc.get_x();
    const auto worker_y = loc.get_y();

    // Try build.
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto probe_x = worker_x + constants::DX[i];
      const auto probe_y = worker_y + constants::DY[i];

      if (!game_state.map_info.is_valid_location(probe_x, probe_y)) continue;
      if (!game_state.my_units.is_occupied[probe_x][probe_y]) continue;
      const auto unit_id = game_state.my_units.by_location[probe_x][probe_y];
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
      const auto probe_x = worker_x + constants::DX[i];
      const auto probe_y = worker_y + constants::DY[i];

      if (!game_state.map_info.is_valid_location(probe_x, probe_y)) continue;
      if (!game_state.my_units.is_occupied[probe_x][probe_y]) continue;
      const auto unit_id = game_state.my_units.by_location[probe_x][probe_y];
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
    const auto loc = game_state.my_units.by_id[worker_id].second;
    const auto worker_x = loc.get_x();
    const auto worker_y = loc.get_y();

    const auto MAX_OBSTRUCTIONS = 3;
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto probe_x = worker_x + constants::DX[i];
      const auto probe_y = worker_y + constants::DY[i];

      if (game_state.count_obstructions(probe_x, probe_y) >= MAX_OBSTRUCTIONS)
        continue;

      if (!game_state.is_safe_location(probe_x, probe_y, 1)) continue;

      const auto dir = static_cast<Direction>(i);
      if (game_state.gc.can_blueprint(worker_id, unit_type, dir)) {
        game_state.blueprint(worker_id, unit_type, dir);
        return true;
      }
    }

    return false;
  }
};

class WorkerRushStrategy : public WorkerStrategy {
 protected:
  const PairwiseDistances distances;

 public:
  WorkerRushStrategy(const PairwiseDistances &distances)
      : distances(distances) {}

  bool run(GameState &game_state, unordered_set<unsigned> workers) {
    vector<MapLocation> target_locations;

    if (should_move_to_enemy) {
      for (const auto &unit : game_state.enemy_units.by_id) {
        const auto loc = unit.second.second;
        target_locations.push_back(loc);
      }
    }

    if (should_move_to_unbuilt_rockets || should_move_to_rockets) {
      for (const auto rocket_id : game_state.my_units.by_type[Rocket]) {
        if (!should_move_to_rockets) {
          const auto rocket_unit = game_state.gc.get_unit(rocket_id);
          if (rocket_unit.structure_is_built()) continue;
        }

        const auto loc = game_state.my_units.by_id[rocket_id].second;
        target_locations.push_back(loc);
      }
    }

    if (should_move_to_unbuilt_factories || should_move_to_factories) {
      for (const auto factory_id : game_state.my_units.by_type[Factory]) {
        if (!should_move_to_factories) {
          const auto factory_unit = game_state.gc.get_unit(factory_id);
          if (factory_unit.structure_is_built()) continue;
        }

        const auto loc = game_state.my_units.by_id[factory_id].second;
        target_locations.push_back(loc);
      }
    }

    unordered_map<uint16_t, unsigned> n_max_targetting;
    for (const auto &loc : target_locations) {
      const auto x = loc.get_x();
      const auto y = loc.get_y();
      const uint16_t hash = (x << 8) + y;
      if (game_state.map_info.can_sense[x][y]) {
        n_max_targetting[hash] = constants::N_DIRECTIONS_WITHOUT_CENTER -
                                 game_state.count_obstructions(x, y) + 1;
      } else {
        n_max_targetting[hash] = 10;
      }
    }

    if (should_move_to_karbonite) {
      for (int x = 0; x < game_state.map_info.width; x++) {
        for (int y = 0; y < game_state.map_info.height; y++) {
          if (game_state.map_info.karbonite[x][y]) {
            // Check if already being targeted.
            const uint16_t hash = (x << 8) + y;
            if (n_max_targetting.count(hash)) continue;

            const auto loc = game_state.map_info.get_location(x, y);
            target_locations.push_back(loc);
            n_max_targetting[hash] = 1;
          }
        }
      }
    }

    const auto targets =
        find_targets(game_state, workers, target_locations, distances);

    unordered_map<uint16_t, unsigned> n_targetting;
    unordered_set<unsigned> targetting;

    // Move towards target.
    for (const auto &target : targets) {
      if (target.distance == numeric_limits<uint16_t>::max()) continue;

      const uint16_t hash = (target.x << 8) + target.y;
      if (n_targetting[hash] >= n_max_targetting[hash]) continue;
      if (targetting.count(target.id)) continue;

      const auto goal = game_state.map_info.get_location(target.x, target.y);
      if (game_state.is_surrounded(goal) && target.distance > 1) continue;

      n_targetting[hash]++;
      targetting.insert(target.id);

      const auto unit_loc = game_state.my_units.by_id[target.id].second;
      const auto unit_x = unit_loc.get_x();
      const auto unit_y = unit_loc.get_y();
      const auto should_move =
          should_move_worker(game_state, unit_x, unit_y, target.x, target.y);

      maybe_move_and_replicate(game_state, target.id, goal, distances,
                               should_move, should_replicate);
    }

    // Explore.
    vector<unsigned> units_to_be_moved_randomly =
        random_move_order(game_state, workers, targetting);
    for (const auto id : units_to_be_moved_randomly) {
      maybe_move_and_replicate_randomly(game_state, id, true, should_replicate);
    }

    return true;
  }

 protected:
  bool should_move_worker(GameState &game_state, unsigned unit_x,
                          unsigned unit_y, unsigned target_x,
                          unsigned target_y) {
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      const auto x = unit_x + constants::DX[i];
      const auto y = unit_y + constants::DY[i];

      if (!game_state.map_info.is_valid_location(x, y)) continue;
      if (x == target_x && y == target_y) return true;

      if (game_state.enemy_units.is_occupied[x][y]) {
        const auto enemy_loc = game_state.map_info.get_location(x, y);
        if (game_state.is_surrounded(enemy_loc)) return false;
      }
    }

    return true;
  }

  vector<unsigned> random_move_order(GameState &game_state,
                                     unordered_set<unsigned> workers,
                                     unordered_set<unsigned> unmovable) {
    vector<vector<bool>> visited(
        game_state.map_info.width,
        vector<bool>(game_state.map_info.height, false));
    queue<pair<int, int>> q;
    vector<unsigned> units_to_be_moved;

    vector<pair<int, int>> initial;

    for (int i = 0; i < game_state.map_info.width; i++) {
      for (int j = 0; j < game_state.map_info.height; j++) {
        if (!game_state.has_unit_at(i, j) &&
            game_state.map_info.passable_terrain[i][j]) {
          visited[i][j] = true;
          initial.push_back(make_pair(i, j));
        }
      }
    }

    random_shuffle(initial.begin(), initial.end());

    for (size_t i = 0; i < initial.size(); i++) {
      q.push(initial[i]);
    }

    while (!q.empty()) {
      pair<int, int> current = q.front();
      q.pop();

      int ii = current.first;
      int jj = current.second;

      for (int k = 0; k < constants::N_DIRECTIONS_WITHOUT_CENTER; k++) {
        int x = ii + constants::DX[k];
        int y = jj + constants::DY[k];

        if (x >= 0 && x < game_state.map_info.width && y >= 0 &&
            y < game_state.map_info.width && !visited[x][y] &&
            game_state.my_units.is_occupied[x][y]) {
          auto id = game_state.my_units.by_location[x][y];
          if (unmovable.count(id) != 0) continue;
          if (!workers.count(id)) continue;

          if (!game_state.gc.is_move_ready(id)) continue;

          units_to_be_moved.push_back(id);
          q.push(make_pair(x, y));
          visited[x][y] = true;
        }
      }
    }
    return units_to_be_moved;
  }
};

class BuildingStrategy : public WorkerStrategy {
 protected:
  const UnitType unit_type;

 public:
  BuildingStrategy(const UnitType unit_type) : unit_type(unit_type) {}

  bool run(GameState &game_state, unordered_set<unsigned> workers) {
    // FIXME: At least build randomly / safely instead of first that can.

    for (const auto worker_id : workers) {
      if (maybe_blueprint(game_state, worker_id, unit_type)) return true;
    }
    return false;
  }
};

class RocketBoardingStrategy : public RobotStrategy {
 public:
  bool run(GameState &game_state, unordered_set<unsigned> robots) {
    auto did_board = false;
    for (const auto robot_id : robots) {
      if (maybe_board_rocket(game_state, robot_id)) did_board = true;
    }
    return did_board;
  }
};

class UnboardingStrategy : public Strategy {
 public:
  bool run(GameState &game_state, unordered_set<unsigned> structures) {
    auto did_unboard = false;
    for (const auto structure_id : structures) {
      const auto structure_unit = game_state.gc.get_unit(structure_id);
      const auto garrison = structure_unit.get_structure_garrison();
      for (int j = 0; j < (int)garrison.size(); j++) {
        MapLocation ml = structure_unit.get_map_location();
        for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
          auto dir = Direction(i);
          if (game_state.gc.can_unload(structure_id, dir)) {
            game_state.unload(structure_id, dir);
            did_unboard = true;
          }
        }
      }
    }
    return did_unboard;
  }
};

class RocketLaunchingStrategy : public Strategy {
 protected:
  const MapInfo mars_map_info;
  const unsigned MIN_UNITS_TO_LAUNCH = 4;

 public:
  RocketLaunchingStrategy(GameState &game_state)
      : mars_map_info(game_state.gc.get_starting_planet(Mars)) {}

  bool run(GameState &game_state, unordered_set<unsigned> rockets) {
    auto did_launch = false;
    for (const auto rocket_id : rockets) {
      // A rocket launch could have destroyed a surrounding rocket.
      if (!game_state.gc.has_unit(rocket_id)) continue;

      const auto rocket_unit = game_state.gc.get_unit(rocket_id);

      if (!rocket_unit.structure_is_built()) continue;

      // Don't launch if insufficient units in garrison unless it's about to
      // flood - in which case launch all the things.
      if (rocket_unit.get_structure_garrison().size() < MIN_UNITS_TO_LAUNCH &&
          game_state.round < constants::FLOOD_ROUND - 1)
        continue;

      const auto ml = mars_map_info.get_random_passable_location();
      if (!game_state.gc.can_launch_rocket(rocket_id, ml)) continue;

      game_state.launch(rocket_id, ml);
      did_launch = true;
    }
    return did_launch;
  }
};

class AttackStrategy : public RobotStrategy {
 protected:
  const UnitType unit_type;
  const unsigned attack_range;
  const PairwiseDistances &distances;

 public:
  AttackStrategy(const UnitType unit_type, const PairwiseDistances &distances)
      : unit_type(unit_type),
        attack_range(constants::ATTACK_RANGE[unit_type]),
        distances(distances) {}

  bool run(GameState &game_state, unordered_set<unsigned> military_units) {
    vector<MapLocation> target_locations;

    for (const auto &unit : game_state.enemy_units.by_id) {
      const auto loc = unit.second.second;
      target_locations.push_back(loc);
    }

    const auto targets =
        find_targets(game_state, military_units, target_locations, distances);

    unordered_map<uint16_t, unsigned> n_targetting;
    unordered_set<unsigned> targetting;

    // Move towards target.
    for (const auto &target : targets) {
      if (target.distance == numeric_limits<uint16_t>::max()) continue;

      const uint16_t hash = (target.x << 8) + target.y;
      if (n_targetting[hash] >= 10) continue;
      if (targetting.count(target.id)) continue;

      const auto goal = game_state.map_info.get_location(target.x, target.y);

      n_targetting[hash]++;
      targetting.insert(target.id);

      maybe_move(game_state, target.id, goal, distances);
    }

    // Attack nearby targets.
    for (const auto militant_id : military_units) {
      if (!targetting.count(militant_id)) {
        maybe_move_randomly(game_state, militant_id);
      }

      const auto loc = game_state.my_units.by_id[militant_id].second;
      auto enemies_within_range = game_state.gc.sense_nearby_units_by_team(
          loc, attack_range, game_state.ENEMY_TEAM);

      sort(enemies_within_range.begin(), enemies_within_range.end(),
           [](const auto &a, const auto &b) {
             return a.get_health() < b.get_health();
           });

      for (const Unit &enemy : enemies_within_range) {
        const auto enemy_id = enemy.get_id();
        if (game_state.gc.is_attack_ready(militant_id) &&
            game_state.gc.can_attack(militant_id, enemy_id)) {
          game_state.attack(militant_id, enemy_id);
        }
      }
    }

    return game_state.enemy_units.all.size() == 0;
  }
};

class HealingStrategy : public RobotStrategy {
 protected:
  const PairwiseDistances &distances;
  const unsigned healing_range = constants::ATTACK_RANGE[Healer];

 public:
  HealingStrategy(const PairwiseDistances &distances) : distances(distances) {}

  bool run(GameState &game_state, unordered_set<unsigned> healers) {
    vector<MapLocation> target_locations;

    for (const auto &unit : game_state.my_units.by_id) {
      // We don't want healers to target themselves or eachother, otherwise they
      // can just ignore other units and clump together since we're sorting by
      // distance.
      const auto id = unit.first;
      if (healers.count(id)) continue;

      const auto loc = unit.second.second;
      target_locations.push_back(loc);
    }

    const auto targets =
        find_targets(game_state, healers, target_locations, distances);

    unordered_map<uint16_t, unsigned> n_targetting;
    unordered_set<unsigned> targetting;

    // Move towards target.
    for (const auto &target : targets) {
      if (target.distance == numeric_limits<uint16_t>::max()) continue;

      const uint16_t hash = (target.x << 8) + target.y;
      if (n_targetting[hash] >= 10) continue;
      if (targetting.count(target.id)) continue;

      const auto goal = game_state.map_info.get_location(target.x, target.y);

      n_targetting[hash]++;
      targetting.insert(target.id);

      maybe_move(game_state, target.id, goal, distances);
    }

    // Heal nearby targets.
    for (const auto healer_id : healers) {
      if (!targetting.count(healer_id)) {
        maybe_move_randomly(game_state, healer_id);
      }

      const auto loc = game_state.my_units.by_id[healer_id].second;
      auto my_units_within_range = game_state.gc.sense_nearby_units_by_team(
          loc, healing_range, game_state.MY_TEAM);

      sort(my_units_within_range.begin(), my_units_within_range.end(),
           [](const auto &a, const auto &b) {
             // Heal lowest health first.
             return a.get_health() < b.get_health();
           });

      for (const Unit &unit : my_units_within_range) {
        const auto unit_id = unit.get_id();

        // To heal, we must call attack()...
        if (game_state.gc.is_attack_ready(healer_id) &&
            game_state.gc.can_attack(healer_id, unit_id)) {
          game_state.attack(healer_id, unit_id);
        }
      }
    }

    return true;
  }
};

class UnitProductionStrategy : public Strategy {
 protected:
  const UnitType unit_type;

 public:
  UnitProductionStrategy(UnitType unit_type) : unit_type(unit_type) {}

  bool run(GameState &game_state, unordered_set<unsigned> factories) {
    for (const auto factory_id : factories) {
      if (game_state.gc.can_produce_robot(factory_id, unit_type)) {
        game_state.produce(factory_id, unit_type);
        return true;
      }
    }
    return false;
  }
};
