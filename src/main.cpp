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

int main() {
  printf("Bot starting...\n");

  // Make matches deterministic.
  srand(0);

  printf("Connecting to manager...\n");

  GameController gc;

  printf("Connected!\n");

  GameState game_state(gc);

  const auto start_s = clock();

  PairwiseDistances point_distances(game_state.map_info.passable_terrain,
                                    constants::POINT_KERNEL);
  PairwiseDistances ranger_attack_distances(
      game_state.map_info.passable_terrain, constants::KERNEL[Ranger]);

  // XXX: this is only ok because they have the same specs.
  PairwiseDistances mage_or_healer_distances(
      game_state.map_info.passable_terrain, constants::KERNEL[Mage]);

  // Strategies.
  WorkerRushStrategy worker_rush(point_distances);
  RocketLaunchingStrategy launch_rockets(game_state);
  RocketBoardingStrategy board_rockets{};
  UnboardingStrategy unboard{};
  array<Strategy*, constants::N_UNIT_TYPES> build = {{
      new UnitProductionStrategy(Worker),
      new UnitProductionStrategy(Knight),
      new UnitProductionStrategy(Ranger),
      new UnitProductionStrategy(Mage),
      new UnitProductionStrategy(Healer),
      new BuildingStrategy(Factory),
      new BuildingStrategy(Rocket),
  }};
  array<Strategy*, constants::N_ROBOT_TYPES> attack = {{
      new NullStrategy(),  // Workers cannot attack.
      new AttackStrategy(Knight, point_distances),
      new AttackStrategy(Ranger, ranger_attack_distances),
      new AttackStrategy(Mage, mage_or_healer_distances),
      new AttackStrategy(Healer, mage_or_healer_distances),
  }};

  const auto stop_s = clock();
  cout << "Analyzing map took "
       << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000 << " milliseconds"
       << endl;

  // First thing get some research going
  if (game_state.PLANET == Earth) {
    gc.queue_research(UnitType::Worker);  // One more karbonite per worker
    gc.queue_research(UnitType::Ranger);  // Faster ranger
    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Worker);  // Increase build speed
    gc.queue_research(UnitType::Rocket);  // To Mars
    gc.queue_research(UnitType::Ranger);  // Larger ranger vision
    gc.queue_research(UnitType::Ranger);  // Snipe
  }

  queue<UnitType> build_queue;

  while (true) {
    game_state.update();

    const auto start_s = clock();
    printf("Round: %d. \n", game_state.round);
    printf("Karbonite: %d. \n", game_state.karbonite);

    // Spam buildings.
    switch (game_state.PLANET) {
      case Earth:
        if (game_state.round % 10 == 0 &&
            game_state.my_units.by_type[Factory].size() < 3) {
          build_queue.push({Factory});
        }

        if (game_state.round > 400 && game_state.round % 20 == 0) {
          build_queue.push({Rocket});
        }

        if (game_state.my_units.by_type[Factory].size()) {
          if (game_state.my_units.by_type[Worker].size() < 8) {
            build_queue.push({Worker});
          } else {
            build_queue.push({Ranger});
          }
        }

        if (build_queue.empty()) {
          worker_rush.set_should_replicate(true);
        } else {
          worker_rush.set_should_replicate(false);
          worker_rush.set_should_move_to_enemy(false);
          const auto unit_type = build_queue.back();

          bool did_something = false;
          switch (unit_type) {
            case Worker:
            case Knight:
            case Ranger:
            case Mage:
            case Healer:
              did_something = build[unit_type]->run(
                  game_state, game_state.my_units.by_type[Factory]);
              break;
            case Rocket:
            case Factory:
              did_something = build[unit_type]->run(
                  game_state, game_state.my_units.by_type[Worker]);
              break;
          }

          if (did_something) {
            build_queue.pop();
          }
        }

        board_rockets.run(game_state, game_state.my_units.all);
        unboard.run(game_state, game_state.my_units.by_type[Factory]);

        break;
      case Mars:
        unboard.run(game_state, game_state.my_units.by_type[Rocket]);
        break;
    }

    worker_rush.run(game_state, game_state.my_units.by_type[Worker]);

    for (int i = 0; i < constants::N_ROBOT_TYPES; i++) {
      const auto unit_type = static_cast<UnitType>(i);
      attack[unit_type]->run(game_state,
                             game_state.my_units.by_type[unit_type]);
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
