#include "GameState.hpp"

GameState::GameState(GameController& gc)
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