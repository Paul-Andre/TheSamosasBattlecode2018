#include "PairwiseDistances.hpp"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <memory>
#include <queue>

using namespace bc;
using namespace std;

PairwiseDistances::PairwiseDistances(vector<vector<bool>> &passable_terrain) {
  distances = (HugeArray *)malloc(sizeof(HugeArray));
  assert(distances != nullptr);
  int n = (int)passable_terrain.size();
  int m = (int)passable_terrain[0].size();
  width = n;
  height = m;
  assert(n <= MAX_MAP_SIZE);
  assert(m <= MAX_MAP_SIZE);

  bool pass[MAX_MAP_SIZE][MAX_MAP_SIZE];
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      pass[i][j] = passable_terrain[i][j];
    }
  }

  bool visited[MAX_MAP_SIZE][MAX_MAP_SIZE];

  int dx[8] = {1, 1, 1, 0, -1, -1, -1, 0};
  int dy[8] = {1, 0, -1, -1, -1, 0, 1, 1};

  memset(distances, -1, sizeof(*distances));

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      if (!pass[i][j]) continue;

      memset(visited, 0, sizeof(visited));

      std::queue<pii> q;

      q.push(pii(i, j));
      visited[i][j] = true;

      unsigned short dist = 0;

      while (!q.empty()) {
        size_t size = q.size();

        for (size_t qi = 0; qi < size; qi++) {
          pii current = q.front();
          q.pop();

          int ii = current.first;
          int jj = current.second;

          (*distances)[i][j][ii][jj] = dist;

          for (int k = 0; k < 8; k++) {
            int x = ii + dx[k];
            int y = jj + dy[k];

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

unsigned short PairwiseDistances::get_distance(int ax, int ay, int bx,
                                               int by) const {
  if ((ax < 0 || ax >= width || ay < 0 || ay >= height) ||
      (bx < 0 || bx >= width || by < 0 || by >= height)) {
    return std::numeric_limits<unsigned short>::max();
  }
  return (*distances)[ax][ay][bx][by];
}
unsigned short PairwiseDistances::get_distance(MapLocation &A,
                                               MapLocation &B) const {
  int ax = A.get_x();
  int ay = A.get_y();
  int bx = B.get_x();
  int by = B.get_y();
  return get_distance(ax, ay, bx, by);
}

PairwiseDistances::~PairwiseDistances() { free(distances); }
