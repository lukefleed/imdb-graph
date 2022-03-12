#!/bin/bash
cd "$(dirname "$(realpath "$0")")"

for votes in  500 1000 5000 10000 25000 50000 75000 100000
do
    echo "##### STARTING FILTERING FOR MIN_MOVIES=$votes #####"
    cd ../../filters
    ./movie_graph_filter.py --votes $votes

    echo "##### STARTING TOP-K CLOSENESS COMPUTATION FOR THE ACTORS GRAPH WITH MIN_MOVIES=$votes #####"
    cd ../scripts/
    /usr/bin/time -o movie_bench/time/top_movies_${votes}_time.log  ./movie_graph movie_bench/top_movies_${votes}
    cd movie_bench

    # echo "##### DONE...\n #####"
done
