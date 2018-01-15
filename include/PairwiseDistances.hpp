#pragma once

#include <vector>
#include "bc.hpp"

using namespace bc;
using namespace std;

constexpr int MAX_MAP_SIZE = 50;

struct PairwiseDistances {
  typedef std::pair<int, int> pii;
  typedef unsigned short HugeArray[MAX_MAP_SIZE][MAX_MAP_SIZE][MAX_MAP_SIZE]
                                  [MAX_MAP_SIZE];

  int width, height;
  HugeArray *distances;

  /// Takes a collision map `coll`
  PairwiseDistances(vector<vector<bool>> &passable_terrain);

  unsigned short get_distance(int ax, int ay, int bx, int by) const;
  unsigned short get_distance(MapLocation &A, MapLocation &B) const;

  ~PairwiseDistances();
};
