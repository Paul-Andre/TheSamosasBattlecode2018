#include "UnitList.hpp"

UnitList::UnitList(GameController& gc, const Team& team)
    : TEAM(team),
      PLANET(gc.get_planet()),
      WIDTH(gc.get_starting_planet(PLANET).get_width()),
      HEIGHT(gc.get_starting_planet(PLANET).get_height()),
      by_location(WIDTH, vector<unsigned>(HEIGHT)),
      is_occupied(WIDTH, vector<bool>(HEIGHT)) {
  const auto planet_map = gc.get_starting_planet(gc.get_planet());
  for (const auto& unit : planet_map.get_initial_units()) {
    const auto id = unit.get_id();
    const auto loc = unit.get_map_location();
    initial_workers[id] = loc;
  }
}

void UnitList::add(unsigned id, UnitType unit_type, MapLocation loc) {
  const auto x = loc.get_x();
  const auto y = loc.get_y();

  by_id[id] = make_pair(unit_type, loc);
  by_type[unit_type].insert(id);
  by_location[x][y] = id;
  is_occupied[x][y] = true;
}

void UnitList::remove(unsigned id) {
  const auto type_loc_pair = by_id[id];
  const auto unit_type = type_loc_pair.first;
  const auto x = type_loc_pair.second.get_x();
  const auto y = type_loc_pair.second.get_y();

  by_id.erase(id);
  by_location[x][y] = -1;
  is_occupied[x][y] = false;
  by_type[unit_type].erase(id);
}

void UnitList::move(unsigned id, Direction dir) {
  const auto type_loc_pair = by_id[id];
  const auto unit_type = type_loc_pair.first;

  const auto prev_loc = type_loc_pair.second;
  const auto prev_x = prev_loc.get_x();
  const auto prev_y = prev_loc.get_y();

  const auto curr_loc = prev_loc.add(dir);
  const auto curr_x = curr_loc.get_x();
  const auto curr_y = curr_loc.get_y();

  by_id[id] = make_pair(unit_type, curr_loc);
  by_location[prev_x][prev_y] = -1;
  is_occupied[prev_x][prev_y] = false;

  by_location[curr_x][curr_y] = id;
  is_occupied[curr_x][curr_y] = true;
}

void UnitList::update(GameController& gc) {
  by_id.clear();

  for (int i = 0; i < constants::N_UNIT_TYPES; i++) {
    by_type[i].clear();
  }

  for (int i = 0; i < WIDTH; i++) {
    for (int j = 0; j < HEIGHT; j++) {
      MapLocation ml(PLANET, i, j);

      is_occupied[i][j] = false;
      by_location[i][j] = -1;

      if (gc.can_sense_location(ml)) {
        if (gc.has_unit_at_location(ml)) {
          auto unit = gc.sense_unit_at_location(ml);
          if (unit.get_team() == TEAM) {
            const auto id = unit.get_id();
            add(id, unit.get_unit_type(), ml);
            if (initial_workers.count(id)) {
              initial_workers.erase(id);
            }
          }
        }
      }
    }
  }

  // Insert initial units that haven't been seen yet.
  unordered_set<unsigned> to_erase;
  for (const auto& worker : initial_workers) {
    const auto id = worker.first;
    const auto loc = worker.second;
    add(id, Worker, loc);
  }
}