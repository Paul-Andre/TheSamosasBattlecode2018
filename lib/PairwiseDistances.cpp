#include "PairwiseDistances.hpp"
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <queue>

using namespace bc;
using namespace std;

PairwiseDistances::PairwiseDistances(
    const vector<vector<bool>> &passable_terrain,
    const vector<PairwiseDistances::pii> &kernel) {
  distances = (HugeArray *)malloc(sizeof(HugeArray));
  assert(distances != nullptr);
  int n = (int)passable_terrain.size();
  int m = (int)passable_terrain[0].size();
  width = n;
  height = m;
  assert(n <= constants::MAX_MAP_SIZE);
  assert(m <= constants::MAX_MAP_SIZE);

  bool pass[constants::MAX_MAP_SIZE][constants::MAX_MAP_SIZE];
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      pass[i][j] = passable_terrain[i][j];
    }
  }

  bool visited[constants::MAX_MAP_SIZE][constants::MAX_MAP_SIZE];

  memset(distances, -1, sizeof(*distances));

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      if (!pass[i][j]) continue;

      memset(visited, 0, sizeof(visited));

      std::queue<pii> q;

      for (int k = 0; k < (int)kernel.size(); k++) {
        int x = i + kernel[k].first;
        int y = j + kernel[k].second;

        if (x >= 0 && x < n && y >= 0 && y < m && !visited[x][y] &&
            pass[x][y]) {
          q.push(pii(x, y));
          visited[x][y] = true;
        }
      }

      unsigned short dist = 0;

      while (!q.empty()) {
        size_t size = q.size();

        for (size_t qi = 0; qi < size; qi++) {
          pii current = q.front();
          q.pop();

          int ii = current.first;
          int jj = current.second;

          (*distances)[i][j][ii][jj] = dist;

          for (int k = 0; k < constants::N_DIRECTIONS_WITHOUT_CENTER; k++) {
            int x = ii + constants::DX[k];
            int y = jj + constants::DY[k];

            if (x >= 0 && x < n && y >= 0 && y < m && !visited[x][y] &&
                pass[x][y]) {
              q.push(pii(x, y));
              visited[x][y] = true;
            }
          }
        }

        dist++;
      }
    }
  }
}

unsigned short PairwiseDistances::get_distance(int start_x, int start_y,
                                               int goal_x, int goal_y) const {
  if ((start_x < 0 || start_x >= width || start_y < 0 || start_y >= height) ||
      (goal_x < 0 || goal_x >= width || goal_y < 0 || goal_y >= height)) {
    return std::numeric_limits<unsigned short>::max();
  }
  return (*distances)[goal_x][goal_y][start_x][start_y];
}

unsigned short PairwiseDistances::get_distance(const MapLocation &start,
                                               const MapLocation &goal) const {
  const auto start_x = start.get_x();
  const auto start_y = start.get_y();
  const auto goal_x = goal.get_x();
  const auto goal_y = goal.get_y();
  return get_distance(start_x, start_y, goal_x, goal_y);
}

PairwiseDistances::~PairwiseDistances() { free(distances); }

// Takes square distances
vector<pair<int, int>> make_kernel(int min_distance_squared,
                                   int max_distance_squared) {
  vector<pair<int, int>> ret;
  int distance_bound = sqrt(max_distance_squared) + 5;
  for (int i = -distance_bound; i <= distance_bound; i++) {
    for (int j = -distance_bound; j <= distance_bound; j++) {
      int distance_squared = i * i + j * j;
      if (distance_squared <= max_distance_squared &&
          distance_squared > min_distance_squared) {
        ret.push_back(make_pair(i, j));
      }
    }
  }
  return ret;
}
