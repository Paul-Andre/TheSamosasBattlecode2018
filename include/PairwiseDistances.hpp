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
  PairwiseDistances(const vector<vector<bool>> &passable_terrain,
                    const vector<pii> &kernel);

  unsigned short get_distance(int ax, int ay, int bx, int by) const;
  unsigned short get_distance(const MapLocation &A, const MapLocation &B) const;

  ~PairwiseDistances();
};

// Takes square distances
vector<pair<int, int>> make_kernel(int min_distance_squared,
                                   int max_distance_squared);
