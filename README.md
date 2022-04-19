# An exact and fast algorithm for computing top-k closeness centrality

The explanation of this algorithm and all it's analysis can be found in the pdf paper

> [Paper](https://github.com/lukefleed/imdb-graph/blob/main/tex/src/main.pdf)


## Documentation

First thing first, we need to clone the repository

```bash
git clone https://github.com/lukefleed/imdb-graph
```

Once done, move in it

```bash
cd imdb-graph
```

### Downloading and filtering the data

All the necessary file are inside the folder `filters`

```bash
cd filters
```
We have two options. If we want to build the graph where the actors are the node, we have to run

```bash
./actors_graph_filter.py --min-movies 42
```

`min-movies` has to ben an integer, `42` is just an example. It represents the minimum number of movies that an actor/actress needs to have done to be considered in our graph.



If we want to build the graph where the movies are the nodes, we have to run

```bash
./movie_graph_filter.py --votes 500
```

`votes` has to ben an integer, `500` is just an example. It represents the minimum number of votes that a movie needs to have on the IMDb database to be considered in our graph.

All the data filtered will be saved in a new folder called `data`

### Running the program

Let's move into the folder `scripts`. If we want to run the program on the actors graph, use

```bash
./actors_graph top_actors_42
```
> IMPORTANT: The algorithm is multi-threaded. It's set with a default number of 12, modify the file .cpp and change this value depending on the CPU.

where `top_actors_42` is the output file name. Anything can be used.

---

If we want to run the program on the movies graph, use

```bash
./movie_graph top_movies_42
```

> IMPORTANT: The algorithm is multi-threaded. It's set with a default number of 12, modify the file .cpp and change this value depending on the CPU.

where `top_movies_42` is the output file name. Anything can be used

---

Those scripts will generate two files .txt (one for the harmonic and one for the closeness centrality). Those files will have the top-100 elements for the relative centrality. If we want a different value, just change the variable `k` in the .cpp files

### Automatic script for different variables of filtering

We are in the folder `scripts`. Inside both the folders `actor-graph` and  `movie-graph` there is a file called `bench_me.sh`. This file will run everything automatically in loop for different values of the filtering variables. To modify this file we need to edit the file. To run it

```bash
./bench_me.sh
```

This will also save the logs in a folder called `time`. It can be usefull to analyze the performance of the program.

---

Inside the folders `closeness centrality` (for both graph), there is a python script `analysis.py`. Put all the generated `_c.txt` files in the folder and run it. It will return a matrix showing the discrepancy of the results while varying the variable


### Generating the interactive graphs

First, let's move into the folder `visualization`

```bash
cd visualization
```
As before, we will find two folders, one for each type of graph. Choose the one that we want to with and move into that folder. Inside it we need to create a folder called `data`

```bash
mkdir data
```

And copy inside it the files

- `Attori.txt`
- `FilmFiltrati.txt`
- `Relazioni.txt`

Attention! If we are visualizing the actors graph, it's important to copy the file generated for it. Ideal values of `min-actors` and `votes` during the filtering are respectively `70` and `100000`. Since it has to be rendered in a web page, this values will generate graphs with about 1000 nodes. I won't suggest to try with bigger graphs

## To Do

- [ ] Organize all the code using `OOP`
- [ ] Normalize the harmonic centrality and it's bound
- [ ] Give `k` as input parameter
