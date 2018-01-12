#include <iostream>
#include <cstdio>
#include <cassert>
#include "../include/bc.hpp"

using namespace std;
using namespace bc;

int main() {
    printf("Player C bot starting\n");

    // It's good to try and make matches deterministic. It's not required, but it
    // makes debugging wayyy easier.
    // Now if you use random() it will produce the same output each map.
    srand(0);

    // we provide some helpful helpers :)
    Direction dir = North;
    Direction opposite = direction_opposite(dir);

    printf("Opposite direction of %d: %d\n", dir, opposite);

    // Make sure that the world is sane!
    assert(opposite == South);

    printf("Connecting to manager...\n");

    // Most methods return pointers; methods returning integers or enums are the only exception.
    GameController gc;

    printf("Connected!\n");

    // loop through the whole game.
    while (true) {
        // The API is "object-oriented" - most methods take pointers to some object.
        uint32_t round = gc.get_round();
        printf("Round: %d\n", round);

        // Note that all operations perform copies out of their data structures, returning new objects.
        // You're responsible for freeing objects.
        vector<Unit> units = gc.get_my_units();

        // it's good to cache things like this. Calls into the API have around a 20ns overhead, plus the cost of
        // copying the data out of the engine. not horrible, but good to avoid more than necessary.
        for (size_t i = 0; i < units.size(); i++) {
            Unit &unit = units[i];

            // Calls on the controller take unit IDs for ownership reasons.
            uint16_t id = unit.get_id();
            if (gc.can_move(id, North) && gc.is_move_ready(id)) {
                gc.move_robot(id, North);
            }

        }

        fflush(stdout);

        gc.next_turn();
    }
}
