#!/usr/bin/env python3
from itertools import combinations
from math import comb
from matplotlib.pyplot import title
import networkx as nx
from pyvis.network import Network

net = Network(height='100%', width='100%', directed=False, bgcolor='#1e1f29', font_color='white')

actors_to_keep = []
with open('data/top_actors_c.txt') as ifs:
    for line in ifs:
        if line.strip():
            actor_id, farness = line.split(maxsplit=1)
            actors_to_keep.append(int(actor_id))

with open('data/Attori.txt') as ifs:
    for line in ifs:
        if line.strip():
            actor_id, actor_name = line.split(maxsplit=1)
            actor_id = int(actor_id)
            if actor_id in actors_to_keep:
                net.add_node(actor_id, label=actor_name)

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

net.set_options("""
var options = {
  "nodes": {
    "borderWidthSelected": 3
  },
  "edges": {
    "color": {
      "inherit": true
    },
    "smooth": false
  },
  "physics": {
    "repulsion": {
      "centralGravity": 8.95,
      "springLength": 500,
      "springConstant": 0.015,
      "nodeDistance": 600,
      "damping": 0.67
    },
    "minVelocity": 0.75,
    "solver": "repulsion"
  }
}
""")

net.show('closeness-graph.html')
