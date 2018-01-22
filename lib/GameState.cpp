#include "GameState.hpp"

GameState::GameState(GameController &gc)
    : TEAM(gc.get_team()),
      PLANET(gc.get_planet()),
      gc(gc),
      round(gc.get_round()),
      karbonite(gc.get_karbonite()),
      map_info(gc.get_starting_planet(PLANET)),
      my_units(gc, TEAM),
      enemy_units(gc, (Team)(1 - TEAM)) {}

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
    int x = target_x + constants::DX[i];
    int y = target_y + constants::DY[i];

    if (!map_info.is_valid_location(x, y)) continue;

    if (!map_info.passable_terrain[x][y]) continue;

    if (!has_unit_at(*map_info.location[x][y])) return false;
  }

  return true;
}

bool GameState::is_surrounding_enemy(const MapLocation &loc) const {
  const auto target_x = loc.get_x();
  const auto target_y = loc.get_y();

  for (int i = 0; i < constants::N_DIRECTIONS_WITHOUT_CENTER; i++) {
    int x = target_x + constants::DX[i];
    int y = target_y + constants::DY[i];

    if (!map_info.is_valid_location(x, y)) continue;

    if (enemy_units.is_occupied[x][y]) return true;
  }

  return false;
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

  // TODO: Update units around the rocket if they were destroyed.
}

void GameState::harvest(unsigned id, Direction dir) {
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
