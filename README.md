# Closeness and Harmonic centrality over the IMDb Graph

**IMPORTANT:** since github does not render the math text, to properly read this README you have to clone the repo locally or install this extension that will render the text

> [GitHub Math Display](https://chrome.google.com/webstore/detail/github-math-display/cgolaobglebjonjiblcjagnpmdmlgmda/related)

---

This project is an exercise realized to implement a Social Network Analysis using the data of the Internet Movie Database (IMDb).

On this data we define an undirected graph $G=(V,E)$ where

- the vertex V are the actor and the actress
- the non oriented vertices in E links the actors and the actresses if they played together in a movie.

The aim of the project was to build a social network over this graph and studying its centralities.

The first challenge was to filter the raw data downloaded from IMDb. One of the first (and funnier) problem was to delete all the actors that works in the Adult industry. They make a lot of movies together and this would have altered the results.

Then, the real challenge has come. We are working with a ton of actors, a brute force approach would have required years to compile: an efficient algorithm was necessary

## Understanding the data

We are taking the data from the official IMDB dataset: https://datasets.imdbws.com/

In particolar we're interest in 3 files

- `title.basics.tsv`
- `title.principals.tsv`
- `name.basics.tsv`

Let's have a closer look to this 3 files:

### title.basics.tsv.gz
_Contains the following information for titles:_
* **tconst** (string) - alphanumeric unique identifier of the title
* **titleType** (string) – the type/format of the title (e.g. movie, short, tvseries, tvepisode, video, etc)
* **primaryTitle** (string) – the more popular title / the title used by the filmmakers on promotional materials at the point of release
* **originalTitle** (string) - original title, in the original language
* **isAdult** (boolean) - 0: non-adult title; 1: adult title
* **startYear** (YYYY) – represents the release year of a title. In the case of TV Series, it is the series start year
* **endYear** (YYYY) – TV Series end year. ‘\N’ for all other title types
* **runtimeMinutes** – primary runtime of the title, in minutes
* **genres** (string array) – includes up to three genres associated with the title

### title.principals.tsv.gz
_Contains the principal cast/crew for titles_
* **tconst** (string) - alphanumeric unique identifier of the title
* **ordering** (integer) – a number to uniquely identify rows for a given titleId
* **nconst** (string) - alphanumeric unique identifier of the name/person
* **category** (string) - the category of job that person was in
* **job** (string) - the specific job title if applicable, else '\N'
* **characters** (string) - the name of the character played if applicable, else '\N'

### name.basics.tsv.gz
_Contains the following information for names:_
* **nconst** (string) - alphanumeric unique identifier of the name/person
* **primaryName** (string)– name by which the person is most often credited
* **birthYear** – in YYYY format
* **deathYear** – in YYYY format if applicable, else '\N'
* **primaryProfession** (array of strings)– the top-3 professions of the person
* **knownForTitles** (array of tconsts) – titles the person is known for



## Filtering

> All This section refers to what's inside the file [filtro.py](https://github.com/lukefleed/imdb-graph/blob/main/filtro.py)

Now that we have downloaded all the files from the dataset, we have to filter them and modify them in order to easly work with them.

### name.basics.tsv

For this file we only need the following columns

- `nconst`
- `primaryTitle`
- `primaryProfession`

Since all the actors starts with the string `nm0` we can remove it to clean the output. Furthermore a lot of actors/actresses do more than one job (director etc..), to avoid excluding important actors we consider all the one that have the string `actor/actress` in their profession. In this way, both someone who is classified as `actor` or as `actor, director` are taken into consideration

```python
df_attori = pd.read_csv(
  'name.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['nconst', 'primaryName', 'primaryProfession'],
  dtype={'primaryName': 'U', 'primaryProfession': 'U'},
  converters={'nconst': lambda x: int(x.lstrip("nm0"))})
df_attori.query('primaryProfession.str.contains("actor") or primaryProfession.str.contains("actress")', inplace=True)
```
Then we can generate the final filtered file `Attori.txt` that has only two columns: `nconst` and `primaryName`

---

### title.basics.tsv.gz

For this file we only need the following columns

- `tconst`
- `primaryTitle`
- `isAdult`
- `titleType`

Since all the movies starts with the string `t0` we can remove it to clean the output. In this case, we also want to remove all the movies for adults.

There are a lot of junk categories considered in IMDb, we are considering all the non adult movies in this whitelist

- `movie`
- `tvSeries`
- `tvMovie`
- `tvMiniSeries`

Why this in particolar? Benefits on the computational cost. There are (really) a lot of single episodes listed in IMDb: to remove them without loosing the most important relations, we only consider the category `tvSeries`. This category list a TV-Series as a single element, not divided in multiple episodes. In this way we will loose some of the relations with minor actors that may appears in just a few episodes. But we will have preserved the relations between the protagonist of the show. _It's not much, but it's an honest work_

```python
print("Filtering films...")
df_film = pd.read_csv(
  'title.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'primaryTitle', 'isAdult', 'titleType'],
  dtype={'primaryTitle': 'U', 'titleType': 'U'},
  converters={'tconst': lambda x: int(x.lstrip("t0")), 'isAdult': lambda x: x != "0"}) #
df_film.query('not isAdult and titleType in ["movie", "tvSeries", "tvMovie", "tvMiniSeries"]',
              inplace=True)
filtered_tconsts = df_film["tconst"].to_list()
```


Then we can generate the final filtered file `FilmFiltrati.txt` that has only two columns: `tconst` and `primaryName`

---

### title.principals.tsv

For this file we only need the following columns

- `tconst`
- `nconst`
- `category`

As before, we clean the output removing unnecessary strings. Then we create an array on unique actor ids (`nconst`) and an array of how may times they appear (`counts`). This will give us the number of movies they appear in. And here it comes the core of this filtering. We define at the start of the algorithm a constant `MIN_MOVIES`. This integer is the minimum number of movies that an actor has to have done in his carrier to be considered in this graph. The reason to do that it's purely computational. If I have to consider all actors the time for the code to compile is the _year(s)'s_ order, that's not good. We are making an approximation: if an actor has less then a reasonable (_42_, as an example) number of movies made in his carrier, there is an high probability that he/she has an important role in our graph during the computation of the centralities.

```python
print("Filtering relations...")
df_relazioni = pd.read_csv(
  'title.principals.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'nconst','category'],
  dtype={'category': 'U'},
  converters={'nconst': lambda x: int(x.lstrip("nm0")), 'tconst': lambda x: int(x.lstrip("t0"))})
df_relazioni.query('(category == "actor" or category == "actress") and tconst in @filtered_tconsts', inplace=True)
nconsts, counts = np.unique(df_relazioni["nconst"].to_numpy(), return_counts=True)
filtered_nconsts = nconsts[counts>=MIN_MOVIES]
df_relazioni.query("nconst in @filtered_nconsts", inplace=True)
```

Notice that we are only selecting actors and actresses that have at least a relation.

```python
print("Re-filtering actors...")
nconsts_with_relations = df_relazioni["nconst"].unique()
df_attori.query("nconst in @nconsts_with_relations", inplace=True)

print("Re-filtering films...")
tconsts_with_relations = df_relazioni["tconst"].unique()
df_film.query("tconst in @tconsts_with_relations", inplace=True)
```

At the end, we can finally generate the file `Relazioni.txt` containing the columns `tconst` and `nconst`

# Understanding the code

Now that we have understood the python code, let's start with the core of the algorithm, written in C++

![](https://i.redd.it/icysmnx0lpsy.jpg)

## Data structures to work with

In this case we are working with tow simple `struct` for the classes _Film_ and _Actor_

```cpp
struct Film {
    string name;
    vector<int> actor_indicies;
};

struct Actor {
    string name;
    vector<int> film_indices;
};
```

Then we need two dictionaries build like this

```cpp
map<int, Actor> A; // Dictionary {actor_id (key): Actor (value)}
map<int, Film> F; // Dictionary {film_id (key): Film (value)}
```

The comments explain everything needed

## Data Read

This section refers to the function `DataRead()`

```cpp
void DataRead()
{
    ifstream actors("data/Attori.txt");
    ifstream movies("data/FilmFiltrati.txt");

    string s,t;
    const string space /* the final frontier */ = "\t";
    for (int i = 1; getline(actors,s); i++)
    {
        if (s.empty())
            continue;
        try {
            Actor TmpObj;
            int id = stoi(s.substr(0, s.find(space)));
            TmpObj.name = s.substr(s.find(space)+1);
            A[id] = TmpObj; // Matlab/Python notation, works with C++17
            if (id > MAX_ACTOR_ID)
                MAX_ACTOR_ID = id;
        } catch (...) {
            cout << "Could not read the line " << i << " of Actors file" << endl;
        }
    }

    for (int i = 1; getline(movies,t); i++)
    {
        if (t.empty())
            continue;

        try{
            Film TmpObj;
            int id = stoi(t.substr(0, t.find(space)));
            TmpObj.name = t.substr(t.find(space)+1);
            F[id] = TmpObj;
        } catch (...) {
            cout << "Could not read the line " << i << " of Film file" << endl;
        }
    }
}
```

We are considering the files `Attori.txt` and `FilmFiltrati.txt`, we don't need the relations one for now. Once that we have read this two files, we loop on each one brutally filling the two dictionaries created before. If a line is empty, we skip it.

## Building the Graph

This section refers to the function `BuildGraph()`

```cpp
void BuildGraph()
{
    ifstream relations("data/Relazioni.txt");
    string s;
    const string space = "\t";

    for (int i=1; getline(relations,s); i++){
        if (s.empty())
            continue;
        try {
            int id_film = stoi(s.substr(0, s.find(space)));
            int id_attore = stoi(s.substr(s.find(space)+1));
            if (A.count(id_attore) && F.count(id_film)) { // Exclude movies and actors filtered
                A[id_attore].film_indices.push_back(id_film);
                F[id_film].actor_indicies.push_back(id_attore);
            }
        } catch (...) {
            cout << "Could not read the line " << i << " of Releations file" << endl;
        }
    }
}
```

In this function, we only ose the file `Relazioni.txt`. As done before, we loop on all the elements of this file, creating

- `id_film`: index key of each movie
- `id_attore`: index key of each actor

Then we exclude the add with `.push_back` this two integers at the end of the vectors of their respective dictionaries. If a line is empty, we skip it.

---

## Closeness Centrality

That's where I tried to experiment a little bit. The original idea to optimize the algorithm was to take a uniformly random subset of actors. This method has a problem: no matter how smart you take this _random_ subset, you are going to exclude some important actors. And I would never want to exclude Ewan McGregor from something!

So I found this [paper](https://arxiv.org/abs/1704.01077) and I decided that this was the way to go

### The problem

Given a connected graph $G = (V, E)$, the closeness centrality of a vertex $v$ is defined as
$$ C(v) = \frac{n-1}{\displaystyle \sum_{\omega \in V} d(v,w)} $$

The idea behind this definition is that a central node should be very efficient in spreading
information to all other nodes: for this reason, a node is central if the average number of links
needed to reach another node is small.

This measure is widely used in the analysis of real-world complex networks, and the problem of selecting the $k$ most central vertices has been deeply analysed in the last decade. However, this problem is computationally not easy, especially for large networks.

This paper proposes a new algorithm that here is implemented to  compute the most central actors in the IMDB collaboration network, where two actors are linked if they played together in a movie.

---

In order to compute the $k$ vertices with largest closeness, the textbook algorithm computes
$c(v)$ for each $v$ and returns the $k$ largest found values. The main bottleneck of this approach
is the computation of $d(v, w)$ for each pair of vertices $v$ and $w$ (that is, solving the All
Pairs Shortest Paths or APSP problem). This can be done in two ways: either by using fast
matrix multiplication, in time $O(n^{2.373} \log n)$ _[Zwick 2002; Williams 2012]_, or by performing _a breadth-first search_ (in short, BFS) from each vertex $v \in V$ , in time $O(mn)$, where $n = |V|$ and $m = |E|$. Usually, the BFS approach is preferred because the other approach contains big constants hidden in the O notation, and because real-world networks are usually sparse, that is, $m$ is not much bigger than $n$. However, also this approach is too time-consuming if the input graph is very big

### Preliminaries

In a connected graph, the farness of a node $v$ in a graph $G = (V,E)$ is
$$ f(v) = \frac{1}{n-1} \displaystyle \sum_{\omega \in V} d(v,w)$$
and the closeness centrality of $v$ is $1/f(v)$ . In the disconnected case, the most natural generalization would be
$$ f(v) = \frac{1}{r(v)-1}\displaystyle \sum_{\omega \in R(v)} d(v,w) $$
and $c(v)=1/f(v)$, where $R(v)$ is the set of vertices reachable from $v$, and $r(v) = |R(v)|$.

But there is a problem:  if $v$ has only one neighbor $w$ at distance $1$, and $w$ has out-degree $0$, then $v$ becomes very central according to this measure, even if $v$ is intuitively peripheral. For this reason, we consider the following generalization, which is quite established in the literature _[Lin 1976; Wasserman and Faust 1994; Boldi and Vigna 2013; 2014; Olsen et al. 2014]:_
$$ f(v) = \frac{n-1}{(r(v)-1)^2}\displaystyle \sum_{\omega \in R(v)} d(v,w) \qquad \qquad c(v)= \frac{1}{f(v)} $$
If a vertex v has (out)degree 0, the previous fraction becomes $\frac{0}{0}$ : in this case, the closeness of $v$ is set to $0$


### The algorithm

In this section, we describe our new approach for computing the k nodes with maximum closeness (equivalently, the $k$ nodes with minimum farness, where the farness $f(v)$ of a vertex is $1/c(v)$ as defined before.

If we have more than one node with the same score, we output all nodes having a centrality bigger than or equal to the centrality of the $k-th$ node.  The basic idea is to keep track of a lower bound on the farness of each node, and to skip the analysis of a vertex $v$ if this lower bound implies that $v$ is not in the _top k_.

More formally, let us assume that we know the farness of some vertices $v_1, ... , v_l$ and a lower bound $L(w)$ on the farness of any other vertex $w$. Furthermore, assume that there
are $k$ vertices among $v_1,...,v_l$ verifying
$$f(v_i) > L(w) \quad \forall ~ w \in V \setminus \{v_1, ..., v_l\}$$
and hence $f(w) \leq L(w) < f (w) ~ ~  \forall w \in V \setminus \{v_1, ..., v_l\}$. Then, we can safely skip the exact computation of $f (w)$ for all remaining nodes $w$, because the $k$ vertices with smallest farness are among $v_1,...,v_l$.

Let's write the Algorithm in pseudo-code, but keep in mind that we will modify it a little bit during the real code.

```cpp
Input : A graph G = (V, E)
Output: Top k nodes with highest closeness and their closeness values c(v)

        global L, Q ← computeBounds(G);
        global Top ← [ ];
        global Farn;

        for v ∈ V do Farn[v] = +∞;
        while Q is not empty do
            v ← Q.extractMin();
            if |Top| ≥ k and L[v] > Farn[Top[k]] then return Top;
            Farn[v] ← updateBounds(v); // This function might also modify L
            add v to Top, and sort Top according to Farn;
            update Q according to the new bounds;
```

- We use a list `TOP` containing all “analysed” vertices $v_1 , ... , v_l$ in increasing order of farness
- We also need a priority queue `Q` containing all vertices _“not analysed, yet”_, in increasing order of lower bound $L$ (this way, the head of $Q$ always has the smallest value of $L$ among all vertices in $Q$).
- At the beginning, using the function computeBounds(), we compute a first bound $L(v)$ for each vertex $v$, and we fill the queue $Q$ according to this bound.
- Then, at each step, we extract the first element $v$ of `Q`: if $L(v)$ is smaller than the _k-th_ biggest farness computed until now (that is, the farness of the _k-th_ vertex in variable `Top`), we can safely stop, because for each $x \in Q, f (x) \leq L(x) \leq L(v) < f (Top[k])$, and $x$ is not in the top $k$.
- Otherwise, we run the function `updateBounds(v)`, which performs a BFS from $v$, returns the farness of $v$, and improves the bounds `L` of all other vertices. Finally, we insert $v$ into `Top` in the right position, and we update `Q` if the lower bounds have changed.

The crucial point of the algorithm is the definition of the lower bounds, that is, the definition of the functions `computeBounds` and `updateBounds`.  Let's define them in a conservative way (due to the fact that I only have a laptop and 16GB of RAM)

- **computeBounds:** The conservative strategy needs time $O(n)$: it simply sets $L(v) = 0$ for each $v$, and it fills `Q` by inserting nodes in decreasing order of degree (the idea is that vertices with high degree have small farness, and they should be analysed as early as possible, so that the values in TOP are correct as soon as possible). Note that the vertices can be sorted in time $O(n)$ using counting sort.

- **updateBounds:** the conservative strategy does not improve `L`, and it cuts the BFS as soon as it is sure that the farness of w is smaller than the k-th biggest farness found until now, that is, `Farn[Top[k]]`. If the BFS is cut, the function returns $+\infty$, otherwise, at the end of the BFS we have computed the farness of $v$, and we can return it. The running time of this procedure is $O(m)$ in the worst case, but it can be much better in practice. It remains to define how the procedure can be sure that the farness of $v$ is at least $x$: to this purpose, during the BFS, we update a lower bound on the farness of $v$. The idea behind this bound is that, if we have already visited all nodes up to distance $d$, we can upper bound the closeness centrality of $v$ by setting distance $d + 1$ to a number of vertices equal to the number of edges “leaving” level $d$, and distance $d + 2$ to all the remaining vertices.

What we are changing in this code is that since $L=0$ is never updated, we do not need to definite it. We will just loop over each vertex, in the order the map prefers. We do not need to define `Q` either, as we will loop over each vertex anyway, and the order does not matter.

The lower bound is

$$ \frac{1}{n-1} (\sigma_{d-1} + n_d \cdot d) $$

where $\sigma$ is the partial sum.

<!-- #### Multi-threaded implementation

We are working on a web-scale graph, multi-threading was a must. At first, we definite a `vector<thread>` and a mutex to prevent simultaneous accesses to the `top_actors` vector. Then preallocate the number of threads we want to use.

```cpp
vector<thread> threads;
mutex top_actors_mutex;
threads.reserve(N_THREADS);
```

Now we can loop con the threads vector and create a vector of booleans `enqueued` to see which vertices we put in the queue during the BFS

```cpp
threads.push_back(thread([&top_actors,&top_actors_mutex,&k](int start) {
vector<bool> enqueued(MAX_ACTOR_ID, false);
```

The we can start looping on each vertex. An import thing to keep in mind is that the actor must exist, otherwise `A[actor_id]` would attempt to write `A`, and this may produce a race condition if multiple threads do it at the same time.

Now let's consider this part of the algorithm explained before

>  if $|Top| \geq k$ and `L[v]` $>$ `Farn[Top[k]]` then return `Top`

This means that we can not exploit the lower bound of our vertex to stop the loop, as we are not updating lower bounds L. We just compute the farness of our vertex using a BFS.

To do that we are using a `FIFO` of pairs `(actor_index, distance from our vector)` and we initialize all the elements of the vector of booleans as _false_. The algorithm needs:

  - `int r = 0`: |R|, where R is the set of vertices reachable from our vertex
  - `long long int sum_distances = 0`: Sum of the distances to other nodes
  - `int prev_distance = 0`: Previous distance, to see when we get to a deeper level of the BFS

Now we can loop on the FIFO structure created before

```cpp
q.push(make_pair(actor_id, 0));
enqueued[actor_id] = true;
bool skip = false;
while (!q.empty()) {
    auto [bfs_actor_id, distance] = q.front();
    q.pop();
```
What we need now is a lower bound on the farness. So if the we find that `distance > prev_distance` we acquire ownership of the mutex, wait if another thread already owns it. Release the mutex when destroyed. Now we are in the first item of the next exploration level, we assume r to have the maximum possibile value (`A.size()`).

Now we can definite the lower bound of the farness:

```cpp
double farness_lower_bound = 1.0 / ((double)A.size() - 1) * (sum_distances + q.size() * distance);
```

Then if this lower bound for the farness is greater than or equal of the _k-1th_ farness, we stop the BFS and destroy `top_actors_lock`, releasing the mutex.

---

Now we have to compute the farness of our vertex `actor_id` (we are still in the `while` that is looping on the FIFO). To do that we consider the integer `bfs_film_id` and loop on its adjacencies and add them to the queue

```cpp
for (int bfs_film_id : A[bfs_actor_id].film_indices) {
    for (int adj_actor_id : F[bfs_film_id].actor_indicies) {
        if (!enqueued[adj_actor_id]) {
        // The adjacent vertices have distance +1 w.r.t. the current vertex
            q.push(make_pair(adj_actor_id, distance+1));
            enqueued[adj_actor_id] = true;
        }
    }
}
``` -->


---

## Harmonic Centrality

The algorithm described before can be easy applied to the harmonic centrality, defined as

$$ h(v) = \sum_{w \in V} \frac{1}{d(v,w)} $$

The main difference here is that we don't have a farness (where small farness implied bigger centrality). Then we won't need a lower bound either. Since the biggest the number is the higher is the centrality we have to adapt the algorithm.

Instead of a lowe bound, we need an upper bound such that

$$ h(v) \leq U_B (v) \leq h(w) $$

We can easily define considering the worst case that could happen at each state:

$$ U_b (v) = \sigma_{d-1} + \frac{n_d}{d} + \frac{n - r - n_d}{d+1}$$

Why this? We are at the level $d$ of our exploration, so we already know the partial sum $\sigma_{d-1}$. The worst case here in this level were we are connected to all the other nodes so we add the other two factors $\frac{n_d}{d} + \frac{n - r - n_d}{d+1}$

Then the algorithm works with the same _top-k_ philosophy, just with an upper bound instead of a lower bound

---

## Benchmarks

Tested on Razer Blade 15 (2018) with an i7-8750H (6 core, 12 thread) and 16GB of DDR4 2666MHz RAM. The algorithm is taking full advantage of all 12 threads

| MIN_ACTORS | k   | Time for filtering | Time to compile |
|------------|-----|--------------------|-----------------|
|42          | 100 | 1m 30s             | 3m 48s          |
|31          | 100 | 1m 44s             | 8m 14s          |
|20          | 100 | 2m 4s              | 19m 34s         |
|15          | 100 | 2m 1s              | 37m 34s         |
| 5          | 100 | 2m 10s             | 2h 52m 57s      |


How the files changes in relation to MIN_ACTORS

| MIN_ACTORS | Attori.txt elements | FilmFiltrati.txt elements | Relazioni.txt elements |
|------------|---------------------|---------------------------|------------------------|
| 42         |  7921               | 266337                    | 545848                 |
| 31         |  13632              | 325087                    | 748580                 |
| 20         |  26337              | 394630                    | 1056544                |
| 15         |  37955              | 431792                    | 1251717                |
|  5         |  126771             | 547306                    | 1949325                |
