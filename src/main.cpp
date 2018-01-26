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

const static int MIN_WORKER_COUNT = 8;
const static int MIN_FACTORY_COUNT = 3;

// Defines distribution of unit types in percentages.
// Should at most add up to 1.
const static array<double, constants::N_UNIT_TYPES> target_distribution = {{
    0.15,  // Worker
    0.70,  // Knight
    0.00,  // Ranger
    0.00,  // Mage
    0.15,  // Healer
    0.00,  // Factory
    0.00,  // Rocket
}};

UnitType which_to_build(const GameState &game_state) {
  const auto worker_count = game_state.my_units.by_type[Worker].size();
  if (worker_count < MIN_WORKER_COUNT) {
    return Worker;
  }

  const auto factory_count = game_state.my_units.by_type[Factory].size();
  if (factory_count < MIN_FACTORY_COUNT) {
    return Factory;
  }

  if (game_state.round >= 400 && game_state.round % 20 == 0) {
    return Rocket;
  }

  const double unit_count = game_state.my_units.all.size();
  array<double, constants::N_UNIT_TYPES> current_distribution_error{};
  for (int i = 0; i < constants::N_UNIT_TYPES; i++) {
    current_distribution_error[i] =
        target_distribution[i] -
        game_state.my_units.by_type[i].size() / unit_count;
  }

  auto max_error = 0.00;
  UnitType unit_to_build = Worker;
  for (int i = 0; i < constants::N_UNIT_TYPES; i++) {
    const auto current_error = current_distribution_error[i];
    if (current_error > max_error) {
      max_error = current_error;
      unit_to_build = static_cast<UnitType>(i);
    }
  }

  return unit_to_build;
}

int main() {
  cout << "Bot starting..." << endl;

#ifdef DEBUG
  // Make matches deterministic when debugging.
  const auto seed = 0;
#else
  const auto seed = time(NULL);
#endif

  srand(seed);
  cout << "Random seed: " << seed << endl;

  cout << "Connecting to manager..." << endl;
  GameController gc;
  cout << "Connected!" << endl;
  fflush(stdout);

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
  array<Strategy *, constants::N_UNIT_TYPES> build = {{
      new UnitProductionStrategy(Worker),
      new UnitProductionStrategy(Knight),
      new UnitProductionStrategy(Ranger),
      new UnitProductionStrategy(Mage),
      new UnitProductionStrategy(Healer),
      new BuildingStrategy(Factory),
      new BuildingStrategy(Rocket),
  }};
  array<Strategy *, constants::N_ROBOT_TYPES> attack = {{
      new NullStrategy(),  // Workers cannot attack.
      new AttackStrategy(Knight, point_distances),
      new AttackStrategy(Ranger, ranger_attack_distances),
      new AttackStrategy(Mage, mage_or_healer_distances),
      new HealingStrategy(mage_or_healer_distances),
  }};

  const auto stop_s = clock();
  cout << "Analyzing map took "
       << (stop_s - start_s) / double(CLOCKS_PER_SEC) * 1000 << " milliseconds"
       << endl;

  // First thing get some research going
  if (game_state.PLANET == Earth) {
    gc.queue_research(UnitType::Worker);  // One more karbonite per worker (25)
    gc.queue_research(UnitType::Knight);  // More defense (25)
    gc.queue_research(UnitType::Knight);  // More defense (75)
    gc.queue_research(UnitType::Knight);  // Javelin (100)
    gc.queue_research(UnitType::Ranger);  // Faster ranger (25)
    gc.queue_research(UnitType::Ranger);  // Larger ranger vision (100)
    gc.queue_research(UnitType::Healer);  // Increase healing (25)
    gc.queue_research(UnitType::Rocket);  // To Mars (50)
    gc.queue_research(UnitType::Healer);  // Increase healing (100)
    gc.queue_research(UnitType::Healer);  // Overcharge (100)
    gc.queue_research(UnitType::Worker);  // Increase build speed (75)
    gc.queue_research(UnitType::Worker);  // Increase build speed (75)
    gc.queue_research(UnitType::Worker);  // Increase build speed (75)
    gc.queue_research(UnitType::Rocket);  // Faster rockets (100)
    gc.queue_research(UnitType::Ranger);  // Snipe (200)
    gc.queue_research(UnitType::Mage);    // Increase attack (25)
    gc.queue_research(UnitType::Mage);    // Increase attack (25)
    gc.queue_research(UnitType::Mage);    // Increase attack (25)
    gc.queue_research(UnitType::Mage);    // Increase attack (25)
    gc.queue_research(UnitType::Rocket);  // Increased capacity (100)
  }

  queue<UnitType> build_queue;

  while (true) {
    game_state.update();

    const auto start_s = clock();
    cout << "Round: " << game_state.round << endl;
    cout << "Karbonite: " << game_state.karbonite << endl;

    switch (game_state.PLANET) {
      case Earth: {
        // Save karbonite.
        worker_rush.set_should_replicate(false);

        if (build_queue.empty()) {
          build_queue.push(which_to_build(game_state));
        }

        auto did_something = false;
        const auto unit_type = build_queue.back();
        switch (unit_type) {
          case Worker:
            if (game_state.my_units.by_type[Worker].size()) {
              // Favor creating workers by replicating because faster.
              worker_rush.set_should_replicate(true);
              did_something = true;
              break;
            } else {
              // NO MORE WORKERS!
              // FALLTHROUGH TO BUILD FROM FACTORY!
            }
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

        board_rockets.run(game_state, game_state.my_units.all);
        unboard.run(game_state, game_state.my_units.by_type[Factory]);
      } break;
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
