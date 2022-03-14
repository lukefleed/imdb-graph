dfs = {
    i: pd.read_csv(f"top_movies_{i:02d}_c.txt", sep='\t', usecols=[1], names=["movie"])
    for i in [500, 1000, 5000, 10000, 25000, 50000, 75000, 100000]}
sets = {i: set(df["movie"]) for i, df in dfs.items()}

diff = []
for i in sets.keys():
    diff.append([len(sets[i]) - len(sets[i] & sets[j]) for j in sets.keys()])
diff = np.array(diff, dtype=float)
diff /= len(next(iter(sets.values())))
