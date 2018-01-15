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

int main() {
  printf("Player C++ bot starting\n");

  // It's good to try and make matches deterministic. It's not required, but it
  // makes debugging wayyy easier.
  // Now if you use random() it will produce the same output each map.
  srand(0);

  printf("Connecting to manager...\n");

  GameController gc;

  printf("Connected!\n");

  PlanetMap initial_planet = gc.get_starting_planet(gc.get_planet());

  Team my_team = gc.get_team();

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
  MapLocation enemy_initial_location;
  for (int i = 0; i < (int)initial_units.size(); i++) {
    Unit &unit = initial_units[i];
    if (unit.get_team() != my_team) {
      enemy_initial_location = unit.get_map_location();
    }
  }

  // First thing get some research going
  if (gc.get_planet() == Earth) {
    gc.queue_research(UnitType::Ranger);  // Faster ranger
    gc.queue_research(UnitType::Worker);  // One more karbonite per worker
    gc.queue_research(UnitType::Ranger);  // Larger ranger vision
    gc.queue_research(UnitType::Rocket);  // To the moon
    gc.queue_research(UnitType::Ranger);  // Snipe
  }

  // loop through the whole game.
  while (true) {
    uint32_t round = gc.get_round();
    int start_s = clock();
    printf("Round: %d. \n", round);
    map_info.update_karbonite(gc);

    // Note that all operations perform copies out of their data structures,
    // returning new objects.

    // Get all units and split into them depending on team and type
    vector<Unit> all_units = gc.get_units();
    vector<vector<Unit>> my_units(8);
    vector<vector<Unit>> enemy_units(8);

    for (int i = 0; i < (int)all_units.size(); i++) {
      // TODO: move them out instead of this copying(?)
      Unit &unit = all_units[i];
      if (unit.get_team() == my_team) {
        my_units[unit.get_unit_type()].push_back(unit);
      } else {
        enemy_units[unit.get_unit_type()].push_back(unit);
      }
    }

    for (size_t i = 0; i < my_units[Factory].size(); i++) {
      Unit &factory = my_units[Factory][i];
      uint16_t id = factory.get_id();
      if (gc.can_produce_robot(id, Ranger)) {
        gc.produce_robot(id, Ranger);
      }

      vector<unsigned int> garrison = factory.get_structure_garrison();
      for (int j = 0; j < (int)garrison.size(); j++) {
        MapLocation ml = factory.get_map_location();
        Direction dir =
            silly_pathfinding(gc, ml, enemy_initial_location, distances);
        if (gc.can_unload(id, dir)) {
          gc.unload(id, dir);
        }
      }
    }

    for (size_t i = 0; i < my_units[Ranger].size(); i++) {
      Unit &unit = my_units[Ranger][i];
      uint16_t id = unit.get_id();

      Location location = unit.get_location();
      if (location.is_on_map()) {
        MapLocation ml = location.get_map_location();
        Direction dir =
            silly_pathfinding(gc, ml, enemy_initial_location, distances);
        if (gc.can_move(id, dir) && gc.is_move_ready(id)) {
          gc.move_robot(id, dir);
        }
      }
    }

    for (size_t i = 0; i < my_units[Worker].size(); i++) {
      Unit &unit = my_units[Worker][i];

      uint16_t id = unit.get_id();

      // Place a blueprint
      bool placed_factory = false;
      for (int dir_int = 0; dir_int < 8; dir_int++) {
        Direction dir = Direction(dir_int);
        if (gc.can_blueprint(id, Factory, dir)) {
          gc.blueprint(id, Factory, dir);
          placed_factory = true;
          break;
        }
      }
      if (placed_factory) continue;

      // TODO: consider the newly places blueprint when considering what
      // other
      // bots should do

      // TODO: having the factory "call" the nearest workers might be a
      // better
      // idea than having the workers decide

      bool moving_towards_factory = false;
      MapLocation goal;
      for (size_t factory_i = 0; factory_i < my_units[Factory].size();
           factory_i++) {
        Unit &factory = my_units[Factory][factory_i];
        if (!factory.structure_is_built()) {
          goal = factory.get_map_location();
          moving_towards_factory = true;
          break;
        }
      }

      if (moving_towards_factory) {
        MapLocation ml = unit.get_map_location();
        Direction dir;

        dir = silly_pathfinding(gc, ml, goal, distances);
        if (gc.can_move(id, dir) && gc.is_move_ready(id)) {
          gc.move_robot(id, dir);
          ml = ml.add(dir);
        }

        dir = silly_pathfinding(gc, ml, goal, distances);
        if (gc.can_replicate(id, dir)) {
          gc.replicate(id, dir);
        }
      } else {
        Direction dir = Direction(rand() % 8);
        if (gc.can_move(id, dir) && gc.is_move_ready(id)) {
          gc.move_robot(id, dir);
        }

        /*
        dir = Direction(rand() % 8);
        if (gc.can_replicate(id, dir)) {
          gc.replicate(id, dir);
        }
        */
      }

      for (size_t factory_i = 0; factory_i < my_units[Factory].size();
           factory_i++) {
        Unit &factory = my_units[Factory][factory_i];
        if (!factory.structure_is_built()) {
          if (gc.can_build(id, factory.get_id())) {
            gc.build(id, factory.get_id());
            break;
          }
        }
      }
    }

    int stop_s = clock();
    cout << "Round took " << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000
         << " milliseconds" << endl;
    fflush(stdout);

    gc.next_turn();
  }
}
