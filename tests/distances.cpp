#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <queue>

using namespace std;

int n, m;

bool coll[50][50];
bool vis[50][50];

short distances[50][50][50][50];

int dx[8] = {1, 1, 1, 0, -1, -1, -1, 0};
int dy[8] = {1, 0, -1, -1, -1, 0, 1, 1};

typedef std::pair<int, int> pii;

int main() {
  memset(distances, -1, sizeof(distances));

  std::cin >> n >> m;

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      char in;
      cin >> in;
      coll[i][j] = (in != '0');
    }
  }

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

        for (int k = 0; k < size; k++) {
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

  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      cout << coll[i][j] << " ";
    }
    cout << endl;
  }

  cout << endl;
  for (int i = 0; i < n; i++) {
    for (int j = 0; j < m; j++) {
      if (distances[i][j][2][2] == -1)
        cout << "* ";
      else
        cout << distances[i][j][2][2] << " ";
    }
    cout << endl;
  }

  std::cerr << "Done\n";
}
