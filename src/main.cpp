#include <cstdio>
#include <ctime>
#include <iostream>
#include <queue>

#include "GameState.hpp"
#include "MapInfo.hpp"
#include "PairwiseDistances.hpp"

#include "bc.hpp"
#include "constants.hpp"
#include "silly_pathfinding.hpp"

using namespace std;
using namespace bc;

enum BuildCommandType {
  BuildRocket,
  BuildFactory,
  ProduceUnit,
};

struct BuildCommand {
  BuildCommandType type;
};

void harvest(GameController &gc, Unit &unit) {
  auto id = unit.get_id();
  Direction best_dir = Center;
  unsigned max_karbonite = 0;
  for (int i = 0; i < constants::N_DIRECTIONS; i++) {
    auto dir = (Direction)i;
    if (gc.can_harvest(id, dir)) {
      auto ml = unit.get_map_location().add(dir);
      if (gc.can_sense_location(ml)) {
        auto karbonite = gc.get_karbonite_at(ml);
        if (karbonite > max_karbonite) {
          max_karbonite = karbonite;
          best_dir = dir;
        }
      }
    }
  }

  if (gc.can_harvest(id, best_dir)) {
    gc.harvest(id, best_dir);
  }
}

void build(GameController &gc, Unit &unit) {
  auto worker_id = unit.get_id();
  auto ml = unit.get_map_location();
  for (int i = 0; i < 9; i++) {
    auto dir = (Direction)i;
    auto loc = ml.add(dir);

    if (!gc.has_unit_at_location(loc)) continue;

    auto other = gc.sense_unit_at_location(loc);

    if (other.get_team() != gc.get_team()) continue;

    auto type = other.get_unit_type();

    if (type != Factory && type != Rocket) continue;

    if (other.structure_is_built()) continue;

    auto struct_id = other.get_id();

    if (!gc.can_build(worker_id, struct_id)) continue;

    gc.build(worker_id, struct_id);

    break;
  }
}

Unit move_worker(GameController &gc, Unit &unit, MapLocation &goal,
                 PairwiseDistances &pd, bool should_replicate) {
  uint16_t id = unit.get_id();
  MapLocation ml = unit.get_map_location();
  Direction dir;

  dir = silly_pathfinding(gc, ml, goal, pd);
  if (gc.can_move(id, dir) && gc.is_move_ready(id)) {
    gc.move_robot(id, dir);
    ml = ml.add(dir);
    unit = gc.get_unit(id);
  }

  if (should_replicate) {
    dir = silly_pathfinding(gc, ml, goal, pd);
    if (gc.can_replicate(id, dir)) {
      gc.replicate(id, dir);
      auto replicated_location = unit.get_map_location().add(dir);
      auto replicated_unit = gc.sense_unit_at_location(replicated_location);
      return replicated_unit;
    } else {
      auto seed = rand();
      for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
        auto dir =
            (Direction)((i + seed) % constants::N_DIRECTIONS_WITHOUT_CENTER);
        if (gc.can_replicate(id, dir)) {
          gc.replicate(id, dir);
          auto replicated_location = unit.get_map_location().add(dir);
          auto replicated_unit = gc.sense_unit_at_location(replicated_location);
          return replicated_unit;
        }
      }
    }
  }

  harvest(gc, unit);
  build(gc, unit);

  return unit;
}

Unit move_worker_randomly(GameController &gc, Unit &unit,
                          bool should_replicate) {
  uint16_t id = unit.get_id();
  MapLocation ml = unit.get_map_location();

  auto seed = rand();
  for (int i = 0; i < 8; i++) {
    auto dir = (Direction)((i + seed) % 8);
    if (gc.can_move(id, dir) && gc.is_move_ready(id)) {
      gc.move_robot(id, dir);
      ml = ml.add(dir);
      unit = gc.get_unit(id);
      break;
    }
  }

  if (should_replicate) {
    seed = rand();
    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      auto dir = (Direction)((i + seed) % 8);
      if (gc.can_replicate(id, dir)) {
        gc.replicate(id, dir);
        auto replicated_location = unit.get_map_location().add(dir);
        auto replicated_unit = gc.sense_unit_at_location(replicated_location);
        return replicated_unit;
      }
    }
  }

  harvest(gc, unit);
  build(gc, unit);

  return unit;
}

void move_unit(GameController &gc, Unit &unit, MapLocation &goal,
               PairwiseDistances &pd) {
  uint16_t id = unit.get_id();
  MapLocation ml = unit.get_map_location();
  Direction dir;

  dir = silly_pathfinding(gc, ml, goal, pd);
  if (gc.can_move(id, dir) && gc.is_move_ready(id)) {
    gc.move_robot(id, dir);
  }
}

void move_unit_randomly(GameController &gc, Unit &unit) {
  uint16_t id = unit.get_id();
  MapLocation ml = unit.get_map_location();

  auto seed = rand();
  for (int i = 0; i < 8; i++) {
    auto dir = (Direction)((i + seed) % 8);
    if (gc.can_move(id, dir) && gc.is_move_ready(id)) {
      gc.move_robot(id, dir);
      break;
    }
  }
}

bool blueprint(MapInfo &map_info, GameController &gc, vector<Unit> my_units,
               UnitType unit_type) {
  for (auto &unit : my_units) {
    auto id = unit.get_id();
    auto loc = unit.get_location();
    if (!loc.is_on_map()) {
      continue;
    }

    auto ml = loc.get_map_location();
    auto unit_x = ml.get_x();
    auto unit_y = ml.get_y();

    for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
      auto x = unit_x + constants::DX[i];
      auto y = unit_y + constants::DY[i];
      if (!map_info.is_valid_location(x, y)) {
        continue;
      }

      bool build_ok = true;

      int distance_from_enemy = 2;
      if (unit_type == Rocket) distance_from_enemy = 2;
      if (unit_type == Factory) distance_from_enemy = 1;

      for (int j = -distance_from_enemy; j <= distance_from_enemy; j++) {
        for (int k = -distance_from_enemy; k <= distance_from_enemy; k++) {
          if (j == 0 && k == 0) continue;

          auto new_x = x + j;
          auto new_y = y + k;
          if (!map_info.is_valid_location(new_x, new_y)) continue;

          auto new_ml = map_info.location[new_x][new_y];
          if (gc.can_sense_location(*new_ml)) {
            if (gc.has_unit_at_location(*new_ml) &&
                gc.sense_unit_at_location(*new_ml).get_team() != gc.get_team())
              build_ok = false;
          }
        }
      }

      // How much enemies and unpassable cells are allowed around
      // TODO: this might not always be possible. In that case build it
      // wherever you can
      int max_obstruction = 4;
      int obstruction = 0;
      for (int k = 0; k < constants::N_DIRECTIONS; k++) {
        int xx = x + constants::DX[k];
        int yy = y + constants::DY[k];

        if (!map_info.is_valid_location(xx, yy)) {
          obstruction++;
          continue;
        }
        if (!map_info.passable_terrain[xx][yy]) obstruction++;
        if (map_info.has_unit[xx][yy]) {
          auto ml = MapLocation(map_info.planet, xx, yy);
          auto other_unit = gc.sense_unit_at_location(ml);
          if (other_unit.get_team() != gc.get_team()) obstruction++;
        }
      }
      if (obstruction > max_obstruction) build_ok = false;

      if (!build_ok) continue;

      auto dir = (Direction)i;
      if (map_info.passable_terrain[x][y]) {
        if (gc.can_blueprint(id, unit_type, dir)) {
          gc.blueprint(id, unit_type, dir);
          return true;
        }
      }
    }
  }

  return false;
}

bool closest_pair_comp(
    const pair<unsigned short, pair<Unit, MapLocation>> &first,
    const pair<unsigned short, pair<Unit, MapLocation>> &second) {
  return first.first < second.first;
}

bool is_surrounded(GameController &gc, MapInfo &map_info,
                   MapLocation &target_location) {
  auto target_x = target_location.get_x();
  auto target_y = target_location.get_y();

  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    int x = target_x + constants::DX[i];
    int y = target_y + constants::DY[i];

    if (!map_info.is_valid_location(x, y)) {
      continue;
    }

    if (!map_info.passable_terrain[x][y]) {
      continue;
    }

    if (!map_info.can_sense[x][y]) {
      return false;
    }

    if (!map_info.has_unit[x][y]) {
      return false;
    }
  }

  return true;
}

bool is_surrounding(GameController &gc, MapInfo &map_info, Unit &unit) {
  if (!unit.get_location().is_on_map()) {
    return false;
  }

  auto ml = unit.get_map_location();
  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    auto dir = (Direction)i;
    auto loc = ml.add(dir);
    auto x = loc.get_x();
    auto y = loc.get_y();

    if (!map_info.is_valid_location(x, y)) continue;

    if (!gc.has_unit_at_location(loc)) continue;

    auto other = gc.sense_unit_at_location(loc);

    if (other.get_team() != gc.get_team()) return true;
  }

  return false;
}

void try_board_nearby_rocket(GameController &gc, Unit &unit) {
  auto worker_id = unit.get_id();
  if (!unit.get_location().is_on_map()) {
    return;
  }

  auto ml = unit.get_map_location();
  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    auto dir = (Direction)i;
    auto loc = ml.add(dir);

    if (!gc.has_unit_at_location(loc)) continue;

    auto other = gc.sense_unit_at_location(loc);

    if (other.get_team() != gc.get_team()) continue;

    if (other.get_unit_type() != Rocket) continue;

    // It's a rocket!
    if (!gc.can_load(other.get_id(), worker_id)) continue;

    gc.load(other.get_id(), worker_id);
  }
}

vector<pair<unsigned short, pair<Unit, MapLocation>>> get_closest_units(
    GameController &gc, MapInfo &map_info, vector<Unit> my_units,
    vector<pair<MapLocation, bool>> target_locations,
    PairwiseDistances &distances) {
  constexpr int MAX_DISTANCES = 10000;
  bool fast_enough = my_units.size() * target_locations.size() < MAX_DISTANCES;

  vector<pair<unsigned short, pair<Unit, MapLocation>>> all_pairs;

  for (int i = 0; i < (int)my_units.size(); i++) {
    unsigned short min_distance = std::numeric_limits<unsigned short>::max();
    auto &my_unit = my_units[i];
    if (!my_unit.get_location().is_on_map()) continue;
    auto surrounding = is_surrounding(gc, map_info, my_unit);

    pair<Unit, MapLocation> min_pair =
        make_pair(my_unit, MapLocation(gc.get_planet(), 0, 0));

    auto ml = my_unit.get_map_location();
    if (fast_enough) {
      for (const auto &target_location_pair : target_locations) {
        auto &target_location = target_location_pair.first;
        auto surrounded = target_location_pair.second;
        auto x = target_location.get_x();
        auto y = target_location.get_y();

        if (surrounding && map_info.can_sense[x][y]) {
          if (!map_info.has_unit[x][y]) continue;
          auto other_unit = gc.sense_unit_at_location(target_location);
          if (other_unit.get_team() == gc.get_team()) continue;
        }

        auto distance = distances.get_distance(ml, target_location);
        if (surrounded && distance > 1) {
          continue;
        }
        if (distance <= min_distance) {
          min_pair = make_pair(my_unit, target_location);
          min_distance = distance;
        }
      }
    } else {
      for (int k = 0; k < constants::N_DIRECTIONS_WITHOUT_CENTER; k++) {
        int some_x = my_unit.get_map_location().get_x() + constants::DX[k];
        int some_y = my_unit.get_map_location().get_y() + constants::DY[k];
        const auto map = gc.get_starting_planet(gc.get_planet());

        if (!map_info.is_valid_location(some_x, some_y)) continue;

        if (map_info.has_unit[some_x][some_y]) {
          auto some_ml = map_info.location[some_x][some_y];
          auto unit = gc.sense_unit_at_location(*some_ml);
          if (unit.get_team() != gc.get_team()) {
            min_pair = make_pair(my_unit, *some_ml);
            min_distance = 1;
          }
        }
      }

      const int N_SAMPLES = MAX_DISTANCES / my_units.size();
      for (int k = 0; k < N_SAMPLES; k++) {
        int j = rand() % target_locations.size();
        auto &target_location = target_locations[j].first;
        auto surrounded = target_locations[j].second;
        auto x = target_location.get_x();
        auto y = target_location.get_y();

        if (surrounding && map_info.can_sense[x][y]) {
          if (!map_info.has_unit[x][y]) continue;
          auto other_unit = gc.sense_unit_at_location(target_location);
          if (other_unit.get_team() == gc.get_team()) continue;
        }

        auto distance = distances.get_distance(ml, target_location);
        if (surrounded && distance > 1) {
          continue;
        }
        if (distance <= min_distance) {
          min_pair = make_pair(my_unit, target_location);
          min_distance = distance;
        }
      }
    }
    all_pairs.push_back(make_pair(min_distance, min_pair));
  }
  sort(all_pairs.begin(), all_pairs.end(), closest_pair_comp);

  return all_pairs;
}

int main() {
  printf("Player C++ bot starting\n");

  // It's good to try and make matches deterministic. It's not required, but it
  // makes debugging wayyy easier.
  // Now if you use random() it will produce the same output each map.
  srand(0);

  printf("Connecting to manager...\n");

  GameController gc;

  printf("Connected!\n");

  const PlanetMap initial_planet = gc.get_starting_planet(gc.get_planet());
  const Team my_team = gc.get_team();

  GameState game_state(gc);
  MapInfo map_info(initial_planet);
  MapInfo mars_map_info(gc.get_starting_planet(Mars));

  int start_s = clock();

  vector<pair<int, int>> point_kernel = {{0, 0}};
  PairwiseDistances point_distances(map_info.passable_terrain, point_kernel);

  // XXX: magic number from the specs
  vector<pair<int, int>> ranger_attack_kernel = make_kernel(10, 50);
  PairwiseDistances ranger_attack_distances(map_info.passable_terrain,
                                            ranger_attack_kernel);

  int stop_s = clock();
  cout << "Analyzing map took "
       << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000 << " milliseconds"
       << endl;

  // Get the location for some bot from the opposite team
  vector<Unit> initial_units = initial_planet.get_initial_units();

  // TODO: enemies might be separated by barriers
  unordered_map<unsigned, Unit> enemy_initial_units;
  for (int i = 0; i < (int)initial_units.size(); i++) {
    Unit &unit = initial_units[i];
    if (unit.get_team() != my_team) {
      enemy_initial_units[unit.get_id()] = unit;
    }
  }

  // First thing get some research going
  if (gc.get_planet() == Earth) {
    gc.queue_research(UnitType::Worker);  // One more karbonite per worker
    gc.queue_research(UnitType::Rocket);  // To mars
    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Ranger);  // Faster ranger
    gc.queue_research(UnitType::Ranger);  // Larger ranger vision
    gc.queue_research(UnitType::Ranger);  // Snipe
  }

  queue<BuildCommand> command_queue;

  // loop through the whole game.
  while (true) {
    game_state.update();
    map_info.update(gc);

    int start_s = clock();
    printf("Round: %d. \n", game_state.round);
    printf("Karbonite: %d. \n", game_state.karbonite);

    // Note that all operations perform copies out of their data structures,
    // returning new objects.

    // Get all units and split into them depending on team and type
    vector<Unit> all_units = gc.get_units();
    vector<vector<Unit>> my_units(constants::N_UNIT_TYPES);
    vector<vector<Unit>> enemy_units(constants::N_UNIT_TYPES);
    vector<MapLocation> target_locations;

    for (int i = 0; i < (int)all_units.size(); i++) {
      // TODO: move them out instead of this copying(?)
      Unit &unit = all_units[i];
      if (unit.get_team() == my_team) {
        my_units[unit.get_unit_type()].push_back(unit);
      } else {
        auto id = unit.get_id();
        enemy_units[unit.get_unit_type()].push_back(unit);
        if (enemy_initial_units.count(id) > 0) {
          enemy_initial_units.erase(id);
        }
        if (unit.get_location().is_on_map()) {
          target_locations.push_back(unit.get_map_location());
        }
      }
    }

    // Put original locations in target
    for (const auto &elem : enemy_initial_units) {
      target_locations.push_back(elem.second.get_map_location());
    }

    // Spam factory buildings.
    if (game_state.round >= 60 && game_state.round % 25 == 11 &&
        game_state.PLANET == Earth && my_units[Factory].size() < 3) {
      command_queue.push({BuildFactory});
    }

    // Check if we have enough karbonite to do the next thing

    bool did_something = true;
    while (!command_queue.empty() && did_something) {
      auto karb = gc.get_karbonite();
      did_something = false;
      if (command_queue.back().type == BuildRocket &&
          karb >= unit_type_get_blueprint_cost(Rocket)) {
        if (blueprint(map_info, gc, my_units[Worker], Rocket)) {
          command_queue.pop();
          did_something = true;
          continue;
        }
      }
      if (command_queue.back().type == BuildFactory &&
          karb >= unit_type_get_blueprint_cost(Factory)) {
        if (blueprint(map_info, gc, my_units[Worker], Factory)) {
          command_queue.pop();
          did_something = true;
          continue;
        }
      }
    }

    if (map_info.planet == Mars) {
      for (size_t i = 0; i < my_units[Rocket].size(); i++) {
        Unit &factory = my_units[Rocket][i];
        uint16_t id = factory.get_id();

        vector<unsigned int> garrison = factory.get_structure_garrison();
        for (int j = 0; j < (int)garrison.size(); j++) {
          MapLocation ml = factory.get_map_location();

          for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
            auto dir = Direction(i);
            if (gc.can_unload(id, dir)) {
              gc.unload(id, dir);
            }
          }
        }
      }
    }

    else {
      for (auto &worker : my_units[Worker]) {
        if (is_surrounding(gc, map_info, worker)) continue;
        try_board_nearby_rocket(gc, worker);
      }

      for (auto &rocket : my_units[Rocket]) {
        if (!rocket.structure_is_built()) continue;
        if (rocket.get_structure_garrison().size() < 4) continue;
        auto ml = mars_map_info.get_random_passable_location();
        if (!gc.can_launch_rocket(rocket.get_id(), *ml)) continue;
        gc.launch_rocket(rocket.get_id(), *ml);
      }

      for (auto &factory : my_units[Factory]) {
        uint16_t id = factory.get_id();
        if (!factory.structure_is_built()) continue;
        vector<unsigned int> garrison = factory.get_structure_garrison();
        for (int j = 0; j < (int)garrison.size(); j++) {
          for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
            auto dir = Direction(i);
            if (gc.can_unload(id, dir)) {
              gc.unload(id, dir);
            }
          }
        }
        if (gc.can_produce_robot(id, Ranger)) {
          gc.produce_robot(id, Ranger);
        }
      }

      // TODO: Remove the need for this.
      // We need to update this since workers might have moved after boarding.
      map_info.update(gc);
    }

    // Decide where to move rangers
    vector<pair<MapLocation, bool>> target_locations_for_rangers;
    for (int i = 0; i < (int)target_locations.size(); i++) {
      auto ml = target_locations[i];
      target_locations_for_rangers.push_back(make_pair(ml, false));
    }
    auto ranger_conquering_pairs = get_closest_units(
        gc, map_info, my_units[Ranger], target_locations_for_rangers,
        ranger_attack_distances);

    // Move rangers (and potentially all military units)
    for (auto &unit_conquering_pair : ranger_conquering_pairs) {
      auto distance = unit_conquering_pair.first;

      Unit &unit = unit_conquering_pair.second.first;
      MapLocation goal = unit_conquering_pair.second.second;

      if (distance == std::numeric_limits<unsigned short>::max()) {
        // goal invalid: ignore and explore.
        move_unit_randomly(gc, unit);
      } else {
        move_unit(gc, unit, goal, ranger_attack_distances);
      }
    }

    // Rangers attack
    for (auto &ranger : my_units[Ranger]) {
      if (ranger.get_location().is_in_space() ||
          ranger.get_location().is_in_garrison())
        continue;
      MapLocation mloc = ranger.get_location().get_map_location();

      auto enemies_within_range =
          gc.sense_nearby_units_by_team(mloc, 50, Team(1 - gc.get_team()));
      for (Unit enemy : enemies_within_range) {
        if (gc.is_attack_ready(ranger.get_id()) &&
            gc.can_attack(ranger.get_id(), enemy.get_id())) {
          gc.attack(ranger.get_id(), enemy.get_id());
        }
      }
    }

    map_info.update(gc);

    // Add rockets for workers
    if (gc.get_planet() == Earth) {
      // Put our rockets into the target vector
      for (int i = 0; i < (int)my_units[Rocket].size(); i++) {
        target_locations.push_back(my_units[Rocket][i].get_map_location());
      }
      for (int i = 0; i < (int)my_units[Factory].size(); i++) {
        if (my_units[Factory][i].structure_is_built() == false) {
          target_locations.push_back(my_units[Factory][i].get_map_location());
        }
      }
    }

    // Get whether enemy units are already surrounded
    vector<pair<MapLocation, bool>> target_locations_and_surrounded;
    for (int i = 0; i < (int)target_locations.size(); i++) {
      auto ml = target_locations[i];
      auto surrounded = is_surrounded(gc, map_info, ml);
      target_locations_and_surrounded.push_back(make_pair(ml, surrounded));
    }

    auto worker_conquering_pairs =
        get_closest_units(gc, map_info, my_units[Worker],
                          target_locations_and_surrounded, point_distances);

    if (worker_conquering_pairs.size() != 0) {
      auto best_distance = worker_conquering_pairs[0].first;

      int replicated_this_turn = 0;

      for (auto &unit_conquering_pair : worker_conquering_pairs) {
        auto distance = unit_conquering_pair.first;

        Unit &replication_unit = unit_conquering_pair.second.first;
        uint16_t replication_id = replication_unit.get_id();
        MapLocation goal = unit_conquering_pair.second.second;

        while (true) {
          bool should_replicate;
          if (command_queue.empty()) {
            if (replicated_this_turn == 0) {
              should_replicate = true;
            } else {
              // Check if we have enough ressources for the closest worker to
              // get to its destination
              auto karb = gc.get_karbonite();
              if (20 * (best_distance / 2) <= karb) {
                should_replicate = true;
              } else {
                should_replicate = false;
              }
            }
          } else {
            should_replicate = false;
          }

          Unit maybe_replicated;
          if (distance == std::numeric_limits<unsigned short>::max()) {
            // goal invalid: ignore and explore.
            maybe_replicated =
                move_worker_randomly(gc, replication_unit, should_replicate);
          } else {
            maybe_replicated = move_worker(gc, replication_unit, goal,
                                           point_distances, should_replicate);
          }
          if (maybe_replicated.get_id() != replication_id) {
            replication_unit = maybe_replicated;
            replication_id = maybe_replicated.get_id();
            replicated_this_turn++;
          } else {
            break;
          }
        }
      }
    }

    cout << "My unit count: " << my_units[Worker].size() << endl;
    cout << "Enemy unit count: " << target_locations_and_surrounded.size()
         << endl;

    int stop_s = clock();
    cout << "Round took " << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000
         << " milliseconds" << endl;
    cout << "Time left " << gc.get_time_left_ms() << endl;
    cout << "==========" << endl;

    fflush(stdout);

    gc.next_turn();
  }
}
