#!/usr/bin/env python

from __future__ import print_function

import random
import sys

N = 50
M = 50

print(N, M)

for _ in range(N):
    for _ in range(M):
        sys.stdout.write(random.choice(["0", "1"]))
    print("")
