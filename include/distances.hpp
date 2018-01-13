#pragma once
#include "bc.hpp"
#include <vector>
#include <memory>
#include <cstdlib>
#include <cstring>
#include <queue>
#include <cassert>
using namespace bc;
using namespace std;



constexpr int MAX_MAP_SIZE = 50;

typedef std::pair<int, int> pii;

struct PairwiseDistances {
  short distances[MAX_MAP_SIZE][MAX_MAP_SIZE][MAX_MAP_SIZE][MAX_MAP_SIZE];
  
  /// Takes a collision map `coll`
  PairwiseDistances(vector<vector<bool> > &coll) {

    int n = (int) coll.size();
    int m = (int) coll[0].size();
    assert(n <= MAX_MAP_SIZE);
    assert(m <= MAX_MAP_SIZE);

    bool vis[50][50];

    int dx[8] = {1, 1, 1, 0, -1, -1, -1, 0};
    int dy[8] = {1, 0, -1, -1, -1, 0, 1, 1};

    memset(distances, -1, sizeof(distances));

    for (int i = 0; i < n; i++) {
      for (int j = 0; j < m; j++) {
        if (coll[i][j]) continue;

        memset(vis, 0, sizeof(vis));

        std::queue<pii> q;

        q.push(pii(i, j));
        vis[i][j] = true;

        short dist = 0;

        while (!q.empty()) {

          size_t size = q.size();

          for (size_t qi = 0; qi < size; qi++) {
            pii current = q.front();
            q.pop();

            int ii = current.first;
            int jj = current.second;

            distances[i][j][ii][jj] = dist;

            for (int k = 0; k < 8; k++) {
              int x = ii + dx[k];
              int y = jj + dy[k];

              if (x >= 0 && x < n && y >= 0 && y < m && !vis[x][y] &&
                  !coll[x][y]) {
                q.push(pii(x, y));
                vis[x][y] = true;
              }
            }
          }

          dist++;
        }
      }
    }
  }

  short get_distance(MapLocation &A, MapLocation &B) {
    return 12312;
    return distances[A.get_x()][A.get_y()][B.get_x()][B.get_y()];
  }
};




