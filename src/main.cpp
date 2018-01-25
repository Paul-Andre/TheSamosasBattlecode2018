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

  // Make matches deterministic.
  srand(0);

  printf("Connecting to manager...\n");

  GameController gc;

  printf("Connected!\n");

  GameState game_state(gc);
  MapInfo mars_map_info(gc.get_starting_planet(Mars));

  const auto start_s = clock();

  PairwiseDistances point_distances(game_state.map_info.passable_terrain,
                                    constants::POINT_KERNEL);
  PairwiseDistances ranger_attack_distances(
      game_state.map_info.passable_terrain, constants::KERNEL[Ranger]);

  WorkerRushStrategy worker_rush(point_distances);
  RocketLaunchingStrategy launch_rockets(game_state);
  BuildingStrategy build_rockets(Rocket);
  BuildingStrategy build_factories(Factory);
  AttackStrategy ranger_attack(Ranger, ranger_attack_distances);
  RocketBoardingStrategy board_rockets{};
  UnboardingStrategy unboard{};

  const auto stop_s = clock();
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
  bool rush_complete = false;
  unsigned round_rush_complete = numeric_limits<unsigned>::max();

  // loop through the whole game.
  while (true) {
    game_state.update();

    const auto start_s = clock();
    printf("Round: %d. \n", game_state.round);
    printf("Karbonite: %d. \n", game_state.karbonite);

    // Spam buildings.
    if ((game_state.round - round_rush_complete) % 50 == 49 &&
        game_state.PLANET == Earth) {
      command_queue.push({BuildRocket});
    }

    if (game_state.PLANET == Mars) {
      unboard.run(game_state, game_state.my_units.by_type[Rocket]);
    }

    if (command_queue.empty()) {
      worker_rush.set_should_replicate(true);
    } else {
      worker_rush.set_should_replicate(false);
      const auto command = command_queue.back();

      bool did_something = false;
      switch (command.type) {
        case BuildRocket:
          did_something = build_rockets.run(
              game_state, game_state.my_units.by_type[Worker]);
          break;
        case BuildFactory:
          did_something = build_rockets.run(
              game_state, game_state.my_units.by_type[Worker]);
          break;
        case ProduceUnit:
          break;
      }

      if (did_something) {
        command_queue.pop();
      }
    }

    board_rockets.run(game_state, game_state.my_units.all);

    auto rush_successful =
        worker_rush.run(game_state, game_state.my_units.by_type[Worker]);
    if (rush_successful && !rush_complete) {
      rush_complete = true;
      round_rush_complete = game_state.round;
    }

    launch_rockets.run(game_state, game_state.my_units.by_type[Rocket]);

    cout << "My unit count: " << game_state.my_units.by_id.size() << endl;
    cout << "Enemy unit count: " << game_state.enemy_units.by_id.size() << endl;

    const auto stop_s = clock();
    cout << "Round took " << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000
         << " ms" << endl;
    cout << "Time left " << gc.get_time_left_ms() << " ms" << endl;
    cout << "==========" << endl;

    fflush(stdout);

    gc.next_turn();
  }
}
