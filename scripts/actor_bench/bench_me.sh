#!/bin/bash
cd "$(dirname "$(realpath "$0")")"

for minmovies in 5 10 20 30 40 50 60 70
do
    echo "##### STARTING FILTERING FOR MIN_MOVIES=$minmovies #####"
    cd ../../filters
    ./actors_graph_filter.py --min-movies $minmovies

    echo "##### STARTING TOP-K CLOSENESS COMPUTATION FOR THE ACTORS GRAPH WITH MIN_MOVIES=$minmovies #####"
    cd ../scripts/
    /usr/bin/time -o actor_bench/time/top_actors_${minmovies}_time.log  ./actors_graph actor_bench/top_actors_${minmovies}
    cd actor_bench

    # echo "##### DONE...\n #####"
done
