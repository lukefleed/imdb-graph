#!/usr/bin/env python3
import os
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

dfs = {
    i: pd.read_csv(f"top_movies_{i:02d}_c.txt", sep='\t', usecols=[1], names=["movie"])
    for i in [500, 1000, 5000, 10000, 25000, 50000, 75000, 100000]}
sets = {i: set(df["movie"]) for i, df in dfs.items()}

diff = []
for i in sets.keys():
    diff.append([len(sets[i]) - len(sets[i] & sets[j]) for j in sets.keys()])
diff = np.array(diff, dtype=float)
diff /= len(next(iter(sets.values())))

plt.matshow(diff)
for (i, j), z in np.ndenumerate(diff):
    plt.gca().text(j, i, f'{z:0.2f}', ha='center', va='center')
plt.gca().set_xticks(np.linspace(0.0, len(sets) - 1, len(sets)))
plt.gca().set_yticks(np.linspace(0.0, len(sets) - 1, len(sets)))
plt.gca().set_xticklabels([f"{i:d}" for i in sets.keys()])
plt.gca().set_yticklabels([f"{i:d}" for i in sets.keys()])
plt.ylabel("\nNumber of Votes")
plt.xlabel("\nNumber of Votes")
cb = plt.colorbar()
cb.set_label("\npercentace of difference in results varing the number of votes")
plt.show()
