#!/usr/bin/env python3
import os
import matplotlib.pyplot as plt
import numpy as np
import pandas as pd

dfs = {
    i: pd.read_csv(f"top_actors_{i:02d}_c.txt", sep='\t', usecols=[1], names=["actor"])
    for i in [5] + list(range(10, 71, 10))}
sets = {i: set(df["actor"]) for i, df in dfs.items()}

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
plt.ylabel("\nMIN_ACTORS value")
plt.xlabel("\nMIN_ACTORS value")
cb = plt.colorbar()
cb.set_label("\npercentace of difference in results varing MIN_MOVIES")
plt.show()
