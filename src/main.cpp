#include <cstdio>
#include <ctime>
#include <iostream>

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
const static int MIN_FACTORY_COUNT = 2;

// Defines distribution of unit types in percentages.
// Should at most add up to 1.
const static array<double, constants::N_UNIT_TYPES> target_distribution = {{
    0.10,  // Worker
    0.35,  // Knight
    0.35,  // Ranger
    0.00,  // Mage
    0.10,  // Healer
    0.08,  // Factory
    0.02,  // Rocket
}};

bool waiting_to_build_rocket = false;

bool is_being_built(const GameState &game_state, UnitType unit_type) {
  for (const auto unit_id : game_state.my_units.by_type[unit_type]) {
    const auto unit = game_state.gc.get_unit(unit_id);
    if (!unit.structure_is_built()) return true;
  }
  return false;
}

UnitType which_to_build(const GameState &game_state) {
  const auto worker_count = game_state.my_units.by_type[Worker].size();
  if (waiting_to_build_rocket && worker_count >= 1) {
    return Rocket;
  }

  if (!is_being_built(game_state, Rocket)) {
    if (game_state.round >= 400 && game_state.round % 50 == 0) {
      waiting_to_build_rocket = true;
      return Rocket;
    }
  }

  if (worker_count < MIN_WORKER_COUNT) {
    return Worker;
  }

  if (!is_being_built(game_state, Factory)) {
    const auto factory_count = game_state.my_units.by_type[Factory].size();
    if (factory_count < MIN_FACTORY_COUNT) {
      return Factory;
    }
  }

  double sum = 0;
  const double unit_count = game_state.my_units.all.size();
  array<double, constants::N_UNIT_TYPES> current_distribution_error{};
  for (int i = 0; i < constants::N_UNIT_TYPES; i++) {
    const auto error =
        max(0.00, target_distribution[i] -
                      game_state.my_units.by_type[i].size() / unit_count);
    current_distribution_error[i] = error;
    sum += error;
  }

  // Sample based on error from distribution.
  auto prob = (double)rand() / RAND_MAX;
  for (int i = 0; i < constants::N_UNIT_TYPES; i++) {
    const auto error_prob = current_distribution_error[i] / sum;
    if (error_prob >= prob) {
      return static_cast<UnitType>(i);
    }
    prob -= error_prob;
  }

  // Shouldn't happen.
  return Worker;
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

  PairwiseDistances worker_distances(game_state.map_info.passable_terrain,
                                     constants::KERNEL[Worker]);
  PairwiseDistances ranger_attack_distances(
      game_state.map_info.passable_terrain, constants::KERNEL[Ranger]);

  // XXX: this is only ok because they have the same specs.
  PairwiseDistances mage_or_healer_distances(
      game_state.map_info.passable_terrain, constants::KERNEL[Mage]);

  // Strategies.
  WorkerRushStrategy worker_rush(worker_distances);
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
  array<RobotStrategy *, constants::N_ROBOT_TYPES> attack = {{
      new NullRobotStrategy(),  // Workers cannot attack.
      new AttackStrategy(Knight, worker_distances),
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
    gc.queue_research(UnitType::Ranger);  // Faster ranger (25)
    gc.queue_research(UnitType::Knight);  // More defense (25)
    gc.queue_research(UnitType::Knight);  // More defense (75)
    gc.queue_research(UnitType::Knight);  // Javelin (100)
    gc.queue_research(UnitType::Ranger);  // Larger ranger vision (100)
    gc.queue_research(UnitType::Rocket);  // To Mars (50)
    gc.queue_research(UnitType::Healer);  // Increase healing (25)
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

  if (game_state.PLANET == Mars) {
    worker_rush.set_should_replicate(true);
    worker_rush.set_should_move_to_rockets(false);
    for (int i = 1; i < constants::N_ROBOT_TYPES; i++) {
      attack[i]->set_should_move_to_rockets(false);
    }
  }

  while (true) {
    game_state.update();

    const auto start_s = clock();
    cout << "Round: " << game_state.round << endl;
    cout << "Karbonite: " << game_state.karbonite << endl;

    switch (game_state.PLANET) {
      case Earth: {
        // Save karbonite.
        worker_rush.set_should_replicate(false);

        auto is_successful = true;
        while (is_successful) {
          is_successful = false;
          const auto unit_type = which_to_build(game_state);
          switch (unit_type) {
            case Worker:
              if (game_state.my_units.by_type[Worker].size() < 3 ||
                  game_state.my_units.by_type[Factory].size() == 0) {
                // Favor creating workers by replicating because faster.
                worker_rush.set_should_replicate(true);
                break;
              } else {
                // NO MORE WORKERS!
                // FALLTHROUGH TO BUILD FROM FACTORY!
              }
            case Knight:
            case Ranger:
            case Mage:
            case Healer:
              is_successful = build[unit_type]->run(
                  game_state, game_state.my_units.by_type[Factory]);
              break;
            case Rocket:
              is_successful = build[unit_type]->run(
                  game_state, game_state.my_units.by_type[Worker]);
              if (is_successful) waiting_to_build_rocket = false;
              break;
            case Factory:
              is_successful = build[unit_type]->run(
                  game_state, game_state.my_units.by_type[Worker]);
              break;
          }
        }

        // Try to build factory anyway to not waste karbonite.
        if (!is_being_built(game_state, Factory) &&
            which_to_build(game_state) != Rocket) {
          build[Factory]->run(game_state, game_state.my_units.by_type[Worker]);
        }

        board_rockets.run(game_state, game_state.my_units.all);
        unboard.run(game_state, game_state.my_units.by_type[Factory]);
      } break;
      case Mars:
        unboard.run(game_state, game_state.my_units.by_type[Rocket]);
        break;
    }

    // Do this twice because might have overcharged.
    for (int i = 0; i < 2; i++) {
      worker_rush.run(game_state, game_state.my_units.by_type[Worker]);
      for (int i = 0; i < constants::N_ROBOT_TYPES; i++) {
        const auto unit_type = static_cast<UnitType>(i);
        attack[unit_type]->run(game_state,
                               game_state.my_units.by_type[unit_type]);
      }
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
