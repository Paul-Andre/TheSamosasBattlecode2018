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

  unsigned short get_distance(int start_x, int start_y, int goal_x,
                              int goal_y) const;
  unsigned short get_distance(const MapLocation &start,
                              const MapLocation &goal) const;

  ~PairwiseDistances();
};

// Takes square distances
vector<pair<int, int>> make_kernel(int min_distance_squared,
                                   int max_distance_squared);
