#include <cstdio>
#include <ctime>
#include <iostream>
#include <queue>

#include "GameState.hpp"
#include "MapInfo.hpp"
#include "PairwiseDistances.hpp"
#include "Strategy.hpp"
#include "TargetSearch.hpp"

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

  sort(all_pairs.begin(), all_pairs.end(),
       [](const auto &a, const auto &b) { return a.first < b.first; });

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
  MapInfo mars_map_info(gc.get_starting_planet(Mars));

  int start_s = clock();

  vector<pair<int, int>> point_kernel = {{0, 0}};
  PairwiseDistances point_distances(game_state.map_info.passable_terrain,
                                    point_kernel);

  // XXX: magic number from the specs
  vector<pair<int, int>> ranger_attack_kernel = make_kernel(10, 50);
  PairwiseDistances ranger_attack_distances(
      game_state.map_info.passable_terrain, ranger_attack_kernel);

  int stop_s = clock();
  cout << "Analyzing map took "
       << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000 << " milliseconds"
       << endl;

  // First thing get some research going
  if (gc.get_planet() == Earth) {
    gc.queue_research(UnitType::Worker);  // One more karbonite per worker
    gc.queue_research(UnitType::Ranger);  // Faster ranger
    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Rocket);  // To Mars

    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Worker);  // Increase build speed
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

    // Spam factory buildings.
    if (game_state.round >= 25 && game_state.round % 15 == 1 &&
        game_state.PLANET == Earth &&
        game_state.my_units.by_type[Factory].size() < 3) {
      command_queue.push({BuildFactory});
    } else if (game_state.round >= 200 && game_state.round % 15 == 1 &&
               game_state.PLANET == Earth) {
      command_queue.push({BuildRocket});
    }

    // Check if we have enough karbonite to do the next thing

    bool did_something = true;
    while (!command_queue.empty() && did_something) {
      auto karb = gc.get_karbonite();
      did_something = false;
      if (command_queue.back().type == BuildRocket &&
          karb >= constants::BLUEPRINT_COST[Rocket]) {
        if (blueprint(map_info, gc, my_units[Worker], Rocket)) {
          command_queue.pop();
          did_something = true;
          continue;
        }
      }
      if (command_queue.back().type == BuildFactory &&
          karb >= constants::BLUEPRINT_COST[Factory]) {
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

      for (auto &ranger : my_units[Ranger]) {
        try_board_nearby_rocket(gc, ranger);
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

        if (my_units[Worker].size() < 8) {
          if (gc.can_produce_robot(id, Worker)) {
            gc.produce_robot(id, Worker);
          }
        } else {
          if (command_queue.empty() && gc.can_produce_robot(id, Ranger)) {
            gc.produce_robot(id, Ranger);
          }
        }
      }

      // TODO: Remove the need for this.
      // We need to update this since workers might have moved after boarding.
      map_info.update(gc);
    }

    // Decide where to move rangers
    const auto &ranger_targets =
        find_targets(game_state, game_state.my_units.by_type[Ranger],
                     target_locations, ranger_attack_distances);

    // Move rangers (and potentially all military units)
    for (auto &target : ranger_targets) {
      if (target.distance == std::numeric_limits<unsigned short>::max()) {
        // goal invalid: ignore and explore.
        move_unit_randomly(game_state, target.id);
      } else {
        const auto loc = game_state.map_info.location[target.x][target.y];
        move_unit(game_state, target.id, *loc, ranger_attack_distances);
      }
    }

    // Rangers attack
    for (auto &ranger : my_units[Ranger]) {
      if (ranger.get_location().is_in_space() ||
          ranger.get_location().is_in_garrison())
        continue;
      MapLocation mloc = ranger.get_location().get_map_location();

      auto enemies_within_range =
          gc.sense_nearby_units_by_team(mloc, 50, game_state.ENEMY_TEAM);
      sort(enemies_within_range.begin(), enemies_within_range.end(),
           [](const Unit &a, const Unit &b) {
             return a.get_health() < b.get_health();
           });
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
