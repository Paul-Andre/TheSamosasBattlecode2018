#pragma once

#include <vector>
#include "bc.hpp"

using namespace bc;
using namespace std;

constexpr int MAX_MAP_SIZE = 50;

struct PairwiseDistances {
  typedef std::pair<int, int> pii;

  short distances[MAX_MAP_SIZE][MAX_MAP_SIZE][MAX_MAP_SIZE][MAX_MAP_SIZE];

  /// Takes a collision map `coll`
  PairwiseDistances(vector<vector<bool>> &coll);

  short get_distance(MapLocation &A, MapLocation &B) const;
};
