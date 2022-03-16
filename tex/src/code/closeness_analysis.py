dfs = {
    i: pd.read_csv(f"top_actors_{i:02d}_c.txt", sep='\t', usecols=[1], names=["actor"])
    for i in [5] + list(range(10, 71, 10))}
sets = {i: set(df["actor"]) for i, df in dfs.items()}

diff = []
for i in sets.keys():
    diff.append([len(sets[i]) - len(sets[i] & sets[j]) for j in sets.keys()])
diff = np.array(diff, dtype=float)
diff /= len(next(iter(sets.values())))

