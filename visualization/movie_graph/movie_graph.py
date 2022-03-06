#!/usr/bin/env python3
from itertools import combinations
from math import comb
import networkx as nx
from pyvis.network import Network

net = Network(height='100%', width='100%', directed=False, bgcolor='#1e1f29', font_color='white')
with open('data/FilmFiltrati.txt') as ifs:
    for line in ifs:
        if line.strip():
            movie_id, movie_name = line.split(maxsplit=1)
            net.add_node(int(movie_id), label=movie_name)

actors = {}  # {movie_id: [actor_id, ...]}
with open('data/Relazioni.txt') as ifs:
    for line in ifs:
        if line.strip():
            movie_id, actor_id = line.split(maxsplit=1)
            actor_id = int(actor_id)
            movie_id = int(movie_id)
            if movie_id not in net.node_ids:
                continue
            if actor_id in actors:
                actors[actor_id].append(movie_id)
            else:
                actors[actor_id] = [movie_id]

edges = set()  # set of unique tuples (actor_id, actor_id)
for actor_id, actors in actors.items():
    actors.sort()
    for movie_id_1, movie_id_2 in combinations(actors, 2):
        edges.add((movie_id_1, movie_id_2))
for movie_id_1, movie_id_2 in edges:
    net.add_edge(movie_id_1, movie_id_2)

# net.hrepulsion(node_distance=500, central_gravity=0.3, spring_length=500, spring_strength=0.05, damping=0.2)
# net.repulsion(node_distance=500, central_gravity=0.3, spring_length=200, spring_strength=0.05, damping=0.2)
# net.show_buttons()

net.set_options(""""
var options = {
  "edges": {
    "color": {
      "inherit": true
    },
    "smooth": false
  },
  "physics": {
    "repulsion": {
      "centralGravity": 0.25,
      "nodeDistance": 500,
      "damping": 0.67
    },
    "maxVelocity": 48,
    "minVelocity": 0.39,
    "solver": "repulsion"
  }
}
""")

net.show('html-files/imdb-graph.html')
