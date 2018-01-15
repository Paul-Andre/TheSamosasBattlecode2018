#include <algorithm>
#include <utility>
#include <vector>
#include "PairwiseDistances.hpp"
#include "bc.hpp"

// TODO: put this in some shared file
// TODO: make sure that this corresponds to the actual direciton values
int dx[9] = {0, 1, 1, 1, 0, -1, -1, -1, 0};
int dy[9] = {1, 1, 0, -1, -1, -1, 0, 1, 0};

// TODO: make some kind of GameState object that I can pass easily
Direction silly_pathfinding(GameController &gc, MapLocation &start,
                            MapLocation &goal, PairwiseDistances &pd) {
  int unit_x = start.get_x();
  int unit_y = start.get_y();

  int x = goal.get_x();
  int y = goal.get_y();

  // Used to sort by distance and then by random
  vector<pair<pair<unsigned short, int>, int>> v;
  for (int k = 0; k < 9; k++) {
    int xx = unit_x + dx[k];
    int yy = unit_y + dy[k];
    v.push_back(make_pair(make_pair(pd.get_distance(x, y, xx, yy), rand()), k));
  }

  sort(v.begin(), v.end());

  for (int i = 0; i < 9; i++) {
    Direction dir = Direction(v[i].second);
    MapLocation new_location = start.add(dir);
    if (gc.can_sense_location(new_location) && gc.is_occupiable(new_location)) {
      return dir;
    }
  }
  return Center;
}
