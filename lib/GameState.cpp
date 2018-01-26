#include "GameState.hpp"

GameState::GameState(GameController &gc)
    : MY_TEAM(gc.get_team()),
      ENEMY_TEAM((Team)(1 - MY_TEAM)),
      PLANET(gc.get_planet()),
      gc(gc),
      round(gc.get_round()),
      karbonite(gc.get_karbonite()),
      map_info(gc.get_starting_planet(PLANET)),
      my_units(gc, MY_TEAM),
      enemy_units(gc, ENEMY_TEAM) {}

void GameState::update() {
  round = gc.get_round();
  karbonite = gc.get_karbonite();
  map_info.update(gc);
  my_units.update(gc);
  enemy_units.update(gc);
}

bool GameState::is_surrounded(const MapLocation &loc) const {
  const auto target_x = loc.get_x();
  const auto target_y = loc.get_y();

  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    const auto x = target_x + constants::DX[i];
    const auto y = target_y + constants::DY[i];

    if (!map_info.is_valid_location(x, y)) continue;

    if (!map_info.passable_terrain[x][y]) continue;

    if (!has_unit_at(map_info.get_location(x, y))) return false;
  }

  return true;
}

bool GameState::is_surrounding_enemy(const MapLocation &loc) const {
  const auto target_x = loc.get_x();
  const auto target_y = loc.get_y();

  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    const auto x = target_x + constants::DX[i];
    const auto y = target_y + constants::DY[i];

    if (!map_info.is_valid_location(x, y)) continue;

    if (enemy_units.is_occupied[x][y]) return true;
  }

  return false;
}

unsigned GameState::count_obstructions(unsigned x, unsigned y) const {
  unsigned obstructions = 0;
  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    const auto probe_x = x + constants::DX[i];
    const auto probe_y = y + constants::DY[i];

    if (!map_info.is_valid_location(probe_x, probe_y)) continue;
    if (enemy_units.is_occupied[probe_x][probe_y]) obstructions++;
    if (!map_info.passable_terrain[probe_x][probe_y]) obstructions++;
  }
  return obstructions;
}

bool GameState::is_safe_location(unsigned x, unsigned y, int radius) const {
  // TODO: Improve based on enemy unit types and their attack ranges.
  for (int i = -radius; i <= radius; i++) {
    for (int j = -radius; j <= radius; j++) {
      const auto probe_x = x + i;
      const auto probe_y = y + j;

      if (!map_info.is_valid_location(probe_x, probe_y)) continue;
      if (enemy_units.is_occupied[probe_x][probe_y]) return false;
    }
  }
  return true;
}

void GameState::move(unsigned id, Direction dir) {
  my_units.move(id, dir);
  gc.move_robot(id, dir);
}

unsigned GameState::blueprint(unsigned id, UnitType unit_type, Direction dir) {
  gc.blueprint(id, unit_type, dir);
  const auto loc = my_units.by_id[id].second.add(dir);
  const auto structure_id = gc.sense_unit_at_location(loc).get_id();
  my_units.add(structure_id, unit_type, loc);
  karbonite = gc.get_karbonite();
  return structure_id;
}

void GameState::load(unsigned structure_id, unsigned robot_id) {
  gc.load(structure_id, robot_id);
  my_units.remove(robot_id);
}

unsigned GameState::unload(unsigned structure_id, Direction dir) {
  gc.unload(structure_id, dir);
  const auto loc = my_units.by_id[structure_id].second.add(dir);
  const auto &robot = gc.sense_unit_at_location(loc);
  const auto robot_id = robot.get_id();
  my_units.add(robot_id, robot.get_unit_type(), loc);
  return robot_id;
}

void GameState::launch(unsigned rocket_id, const MapLocation &loc) {
  gc.launch_rocket(rocket_id, loc);
  my_units.remove(rocket_id);

  // Update units around the rocket if they were destroyed.
  const auto x = loc.get_x();
  const auto y = loc.get_y();
  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    const auto probe_x = x + constants::DX[i];
    const auto probe_y = y + constants::DY[i];

    if (!map_info.is_valid_location(probe_x, probe_y)) continue;
    const auto probe_loc = map_info.get_location(probe_x, probe_y);

    if (gc.can_sense_location(probe_loc) && gc.has_unit_at_location(probe_loc))
      continue;

    if (my_units.is_occupied[probe_x][probe_y]) {
      const auto unit_id = my_units.by_location[probe_x][probe_y];
      if (!gc.has_unit(unit_id)) my_units.remove(unit_id);
    } else if (enemy_units.is_occupied[probe_x][probe_y]) {
      const auto unit_id = enemy_units.by_location[probe_x][probe_y];
      if (!gc.has_unit(unit_id)) enemy_units.remove(unit_id);
    }
  }
}

void GameState::disintegrate(unsigned id) {
  gc.disintegrate_unit(id);
  my_units.remove(id);
}

void GameState::attack(unsigned id, unsigned target_id) {
  gc.attack(id, target_id);
  update_if_dead(target_id);
}

void GameState::special_attack(unsigned id, UnitType unit_type,
                               unsigned target_id) {
  switch (unit_type) {
    case Worker:
      break;
    case Knight:
      if (gc.can_javelin(id, target_id) && gc.is_javelin_ready(id)) {
        gc.javelin(id, target_id);
        update_if_dead(target_id);
      }
      break;
    case Ranger:
      break;
    case Mage:
      break;
    case Healer:
      if (gc.can_overcharge(id, target_id) && gc.is_overcharge_ready(id)) {
        gc.overcharge(id, target_id);
      }
      break;
    default:
      break;
  }
}

void GameState::harvest(unsigned id, Direction dir) {
  gc.harvest(id, dir);
  const auto loc = my_units.by_id[id].second.add(dir);
  const auto x = loc.get_x();
  const auto y = loc.get_y();
  map_info.karbonite[x][y] = gc.get_karbonite_at(loc);
  karbonite = gc.get_karbonite();
}

unsigned GameState::replicate(unsigned id, Direction dir) {
  gc.replicate(id, dir);
  const auto loc = my_units.by_id[id].second.add(dir);
  const auto replicated_id = gc.sense_unit_at_location(loc).get_id();
  my_units.add(replicated_id, Worker, loc);
  karbonite = gc.get_karbonite();
  return replicated_id;
}

void GameState::produce(unsigned factory_id, UnitType unit_type) {
  gc.produce_robot(factory_id, unit_type);
  karbonite = gc.get_karbonite();
}
