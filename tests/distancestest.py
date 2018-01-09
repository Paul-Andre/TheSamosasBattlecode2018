import random
import sys

n = 50
m = 50

print(n,m);

for _ in range(n):
    for _ in range(m):
        sys.stdout.write(random.choice(["0","1"]))
    print()



