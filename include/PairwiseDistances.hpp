#pragma once

#include <vector>
#include "bc.hpp"
#include "constants.hpp"

using namespace bc;
using namespace std;

struct PairwiseDistances {
  typedef std::pair<int, int> pii;
  typedef unsigned short
      HugeArray[constants::MAX_MAP_SIZE][constants::MAX_MAP_SIZE]
               [constants::MAX_MAP_SIZE][constants::MAX_MAP_SIZE];

  int width, height;
  HugeArray *distances;

  /// Takes a collision map `coll`
  PairwiseDistances(vector<vector<bool>> &passable_terrain);

  unsigned short get_distance(int ax, int ay, int bx, int by) const;
  unsigned short get_distance(MapLocation &A, MapLocation &B) const;

  ~PairwiseDistances();
};
