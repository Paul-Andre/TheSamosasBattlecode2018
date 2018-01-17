#include <cassert>
#include <cstdio>
#include <ctime>
#include <iostream>
#include "MapInfo.hpp"
#include "PairwiseDistances.hpp"
#include "bc.hpp"
#include "silly_pathfinding.hpp"

using namespace std;
using namespace bc;

void harvest(GameController &gc, Unit &unit) {
  auto id = unit.get_id();
  Direction best_dir = Center;
  int max_karbonite = 0;
  for (int i = 0; i < 9; i++) {
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
      for (int i = 0; i < 8; i++) {
        auto dir = (Direction)((i + seed) % 8);
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

  return unit;
}

Unit move_worker_randomly(GameController &gc, Unit &unit, bool should_replicate) {
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
    for (int i = 0; i < 8; i++) {
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

  return unit;
}

bool closest_pair_comp(
    const pair<unsigned short, pair<Unit, MapLocation>> &first,
    const pair<unsigned short, pair<Unit, MapLocation>> &second) {
  return first.first < second.first;
}

bool is_surrounded(GameController &gc, MapLocation &target_location) {
  int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
  int dy[] = {1, 1, 0, -1, -1, -1, 0, 1};

  auto x = target_location.get_x();
  auto y = target_location.get_y();

  const auto map = gc.get_starting_planet(gc.get_planet());

  auto width = map.get_width();
  auto height = map.get_height();

  for (int i = 0; i < 8; i++) {
    int some_x = x + dx[i];
    int some_y = y + dy[i];

    if (some_x < 0 || some_x >= width || some_y < 0 || some_y >= height) {
      continue;
    }

    MapLocation ml(gc.get_planet(), some_x, some_y);
    if (!map.is_passable_terrain_at(ml)) {
      continue;
    }

    if (!gc.can_sense_location(ml)) {
      return false;
    }

    if (!gc.has_unit_at_location(ml)) {
      return false;
    }
  }

  return true;
}

vector<pair<unsigned short, pair<Unit, MapLocation>>> get_closest_units(
    GameController &gc, vector<Unit> my_units,
    vector<pair<MapLocation, bool>> target_locations,
    PairwiseDistances &distances) {
  vector<pair<unsigned short, pair<Unit, MapLocation>>> all_pairs;
  const int MAX_DISTANCES = 10000;
  bool fast_enough = my_units.size() * target_locations.size() < MAX_DISTANCES;
  for (int i = 0; i < my_units.size(); i++) {
    unsigned short min_distance = std::numeric_limits<unsigned short>::max();
    auto &my_unit = my_units[i];
    pair<Unit, MapLocation> min_pair =
        make_pair(my_unit, MapLocation(gc.get_planet(), 0, 0));
    if (fast_enough) {
      for (int j = 0; j < target_locations.size(); j++) {
        auto &target_location = target_locations[j].first;
        auto surrounded = target_locations[j].second;
        auto ml = my_unit.get_map_location();
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
      int dx[] = {0, 1, 1, 1, 0, -1, -1, -1};
      int dy[] = {1, 1, 0, -1, -1, -1, 0, 1};
      for (int k = 0; k < 8; k++) {
        int some_x = my_unit.get_map_location().get_x() + dx[k];
        int some_y = my_unit.get_map_location().get_y() + dy[k];
        const auto map = gc.get_starting_planet(gc.get_planet());

        if (some_x < 0 || some_x >= map.get_width() || some_y < 0 ||
            some_y >= map.get_height()) {
          continue;
        }

        MapLocation ml(gc.get_planet(), some_x, some_y);
        if (gc.has_unit_at_location(ml)) {
          auto unit = gc.sense_unit_at_location(ml);
          if (unit.get_team() != gc.get_team()) {
            min_pair = make_pair(my_unit, ml);
            min_distance = 1;
          }
        }
      }

      const int N_SAMPLES = MAX_DISTANCES / my_units.size();
      for (int k = 0; k < N_SAMPLES; k++) {
        int j = rand() % target_locations.size();
        auto &target_location = target_locations[j].first;
        auto surrounded = target_locations[j].second;
        auto ml = my_unit.get_map_location();
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

  MapInfo map_info(initial_planet);

  int start_s = clock();
  PairwiseDistances distances(map_info.passable_terrain);
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
    gc.queue_research(UnitType::Worker);  // One more karbonite per worker
    gc.queue_research(UnitType::Worker);  // One more karbonite per worker
    gc.queue_research(UnitType::Ranger);  // Faster ranger
    gc.queue_research(UnitType::Ranger);  // Larger ranger vision
    gc.queue_research(UnitType::Ranger);  // Snipe
  }

  // loop through the whole game.
  while (true) {
    uint32_t round = gc.get_round();
    int start_s = clock();
    printf("Round: %d. \n", round);
    printf("Karbonite: %d. \n", gc.get_karbonite());
    map_info.update_karbonite(gc);

    // Note that all operations perform copies out of their data structures,
    // returning new objects.

    // Get all units and split into them depending on team and type
    vector<Unit> all_units = gc.get_units();
    vector<vector<Unit>> my_units(8);
    vector<vector<Unit>> enemy_units(8);
    vector<pair<MapLocation, bool>> all_enemy_unit_locations;

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
          auto ml = unit.get_map_location();
          auto surrounded = is_surrounded(gc, ml);
          all_enemy_unit_locations.push_back(make_pair(ml, surrounded));
        }
      }
    }

    for (const auto &elem : enemy_initial_units) {
      auto ml = elem.second.get_map_location();
      auto surrounded = is_surrounded(gc, ml);
      all_enemy_unit_locations.push_back(make_pair(ml, surrounded));
    }

    auto unit_conquering_pairs = get_closest_units(
        gc, my_units[Worker], all_enemy_unit_locations, distances);

    if (unit_conquering_pairs.size() != 0) {

      auto best_distance = unit_conquering_pairs[0].first;

      int replicated_this_turn = 0;

      for (auto &unit_conquering_pair : unit_conquering_pairs) {
        auto distance = unit_conquering_pair.first;

        Unit &replication_unit = unit_conquering_pair.second.first;
        uint16_t replication_id = replication_unit.get_id();
        MapLocation goal = unit_conquering_pair.second.second;

        while (true) {

          bool should_replicate;
          if (replicated_this_turn == 0) {
            should_replicate = true;
          }
          else {
            // Check if we have enough ressources for the closest worker to get to its destination
            auto karb = gc.get_karbonite();
            if (5*(best_distance/2)  <= karb) {
              should_replicate = true;
            }
            else {
              should_replicate = false;
            }
          }

          Unit maybe_replicated;
          if (distance == std::numeric_limits<unsigned short>::max()) {
            // goal invalid: ignore and explore.
            maybe_replicated = move_worker_randomly(gc, replication_unit, should_replicate);
          } else {
            maybe_replicated = move_worker(gc, replication_unit, goal, distances, should_replicate);
          }
          if (maybe_replicated.get_id() != replication_id) {
            replication_unit = maybe_replicated;
            replication_id = maybe_replicated.get_id();
            replicated_this_turn ++;
          } else {
            break;
          }
        }
      }
    }

    cout << "My unit count: " << my_units[Worker].size() << endl;
    cout << "Enemy unit count: " << all_enemy_unit_locations.size() << endl;

    int stop_s = clock();
    cout << "Round took " << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000
         << " milliseconds" << endl;
    cout << "Time left " << gc.get_time_left_ms() << endl;

    fflush(stdout);

    gc.next_turn();
  }
}
