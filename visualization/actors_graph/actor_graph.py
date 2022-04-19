#!/usr/bin/env python3
from itertools import combinations
from math import comb
import networkx as nx
from pyvis.network import Network

net = Network(height='100%', width='100%', directed=False, bgcolor='#1e1f29', font_color='white')
with open('data/Attori.txt') as ifs:
    for line in ifs:
        if line.strip():
            actor_id, actor_name = line.split(maxsplit=1)
            net.add_node(int(actor_id), label=actor_name)

movies = {}  # {movie_id: [actor_id, ...]}
with open('data/Relazioni.txt') as ifs:
    for line in ifs:
        if line.strip():
            movie_id, actor_id = line.split(maxsplit=1)
            actor_id = int(actor_id)
            movie_id = int(movie_id)
            if actor_id not in net.node_ids:
                continue
            if movie_id in movies:
                movies[movie_id].append(actor_id)
            else:
                movies[movie_id] = [actor_id]

edges = set()  # set of unique tuples (actor_id, actor_id)
for movie_id, actors in movies.items():
    actors.sort()
    for actor_id_1, actor_id_2 in combinations(actors, 2):
        edges.add((actor_id_1, actor_id_2))
for actor_id_1, actor_id_2 in edges:
    net.add_edge(actor_id_1, actor_id_2)

# net.hrepulsion(node_distance=500, central_gravity=0.3, spring_length=500, spring_strength=0.05, damping=0.2)
# net.repulsion(node_distance=500, central_gravity=0.3, spring_length=200, spring_strength=0.05, damping=0.2)
# net.show_buttons()

# I suggest to modify this parametres using the GUI
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
