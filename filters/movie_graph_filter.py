#!/usr/bin/env python3
import gzip
import requests
import pandas as pd
import numpy as np
import os
import csv

#-----------------DOWNLOAD .GZ FILES FROM IMDB DATABASE-----------------#
def colored(r, g, b, text):
    return "\033[38;2;{};{};{}m{} \033[38;2;255;255;255m".format(r, g, b, text)

def download_url(url):
  print("Downloading:", url)
  file_name_start_pos = url.rfind("/") + 1
  file_name = url[file_name_start_pos:]
  if os.path.isfile(file_name):
    print(colored(0,170,0,"Already downloaded: skipping"))
    return

  r = requests.get(url, stream=True)
  r.raise_for_status()
  with open(file_name, 'wb') as f:
    for chunk in r.iter_content(chunk_size=4096):
      f.write(chunk)
  return url

urls = ["https://datasets.imdbws.com/name.basics.tsv.gz",
        "https://datasets.imdbws.com/title.principals.tsv.gz",
        "https://datasets.imdbws.com/title.basics.tsv.gz",
        "https://datasets.imdbws.com/title.ratings.tsv.gz"]

for url in urls:
  download_url(url)

os.makedirs("../data/data_movie_graph", exist_ok=True) # Generate (recursively) folders, ignores the comand if they already exists

#------------------------------FILTERING------------------------------#

print("Filtering actors...")
df_attori = pd.read_csv(
  'name.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['nconst', 'primaryName', 'primaryProfession'],
  dtype={'primaryName': 'U', 'primaryProfession': 'U'},
  converters={'nconst': lambda x: int(x.lstrip("nm0"))})
df_attori.query('primaryProfession.str.contains("actor") or primaryProfession.str.contains("actress")', inplace=True)


print("Filtering movies...")
df_film = pd.read_csv(
  'title.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'primaryTitle', 'isAdult', 'titleType'], # Considering only this columns
  dtype={'primaryTitle': 'U', 'titleType': 'U'}, # Both are unsigned integers
  converters={'tconst': lambda x: int(x.lstrip("t0")), 'isAdult': lambda x: x != "0"}) # All movies starts with t0, we are just cleaning the output. Then remove all adult movies
df_ratings = pd.read_csv(
  'title.ratings.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'numVotes'],
  dtype={'numVotes': 'u8'}, # Unsigned integer
  converters={'tconst': lambda x: int(x.lstrip("t0"))})
df_film = pd.merge(df_film, df_ratings, "left", on="tconst")
del df_ratings
df_film.query('not isAdult and titleType in ["movie", "tvSeries", "tvMovie", "tvMiniSeries"]',
              inplace=True)
VOTES_MEAN = df_film['numVotes'].mean()
df_film.query('numVotes > @VOTES_MEAN', inplace=True)
filtered_tconsts = df_film["tconst"].to_list()

print("Filtering relations...")
df_relazioni = pd.read_csv(
  'title.principals.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'nconst','category'],  # Considering only this columns
  dtype={'category': 'U'}, # Unsigned integer
  converters={'nconst': lambda x: int(x.lstrip("nm0")), 'tconst': lambda x: int(x.lstrip("t0"))}) # Cleaning
df_relazioni.query('(category == "actor" or category == "actress") and tconst in @filtered_tconsts', inplace=True)


# Write the filtered files
df_attori.to_csv('../data/data_movie_graph/Attori.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['nconst', 'primaryName'], header=False, index=False)

df_film.to_csv('../data/data_movie_graph/FilmFiltrati.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['tconst', 'primaryTitle'], header=False, index=False)

df_relazioni.to_csv('../data/data_movie_graph/Relazioni.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['tconst', 'nconst'], header=False, index=False)
