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

int main() {
  printf("Player C++ bot starting\n");

  // It's good to try and make matches deterministic. It's not required, but it
  // makes debugging wayyy easier.
  // Now if you use random() it will produce the same output each map.
  srand(0);

  printf("Connecting to manager...\n");

  GameController gc;

  printf("Connected!\n");

  GameState game_state(gc);
  MapInfo mars_map_info(gc.get_starting_planet(Mars));

  int start_s = clock();

  PairwiseDistances point_distances(game_state.map_info.passable_terrain,
                                    constants::POINT_KERNEL);
  PairwiseDistances ranger_attack_distances(
      game_state.map_info.passable_terrain, constants::RANGER_KERNEL);

  WorkerRushStrategy worker_rush(point_distances);
  RocketLaunchingStrategy rocket_launch(game_state);
  RocketBoardingStrategy board_rockets{};
  BuildingStrategy build_rockets(Rocket);
  BuildingStrategy build_factories(Factory);
  AttackStrategy ranger_attack(Ranger, ranger_attack_distances);

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

    int start_s = clock();
    printf("Round: %d. \n", game_state.round);
    printf("Karbonite: %d. \n", game_state.karbonite);

    worker_rush.run(game_state, game_state.my_units.by_type[Worker]);

    cout << "My unit count: " << game_state.my_units.by_id.size() << endl;
    cout << "Enemy unit count: " << game_state.enemy_units.by_id.size() << endl;

    int stop_s = clock();
    cout << "Round took " << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000
         << " milliseconds" << endl;
    cout << "Time left " << gc.get_time_left_ms() << endl;
    cout << "==========" << endl;

    fflush(stdout);

    gc.next_turn();
  }
}
