#!/usr/bin/env python3
import requests
import pandas as pd
import numpy as np
import os
import csv

MIN_MOVIES = 42  # Only keep relations for actors that have made more than this many movies

#-----------------DOWNLOAD .GZ FILES FROM IMDB DATABASE-----------------#

def download_url(url):
  print("Downloading:", url)
  file_name_start_pos = url.rfind("/") + 1
  file_name = url[file_name_start_pos:]
  if os.path.isfile(file_name):
    print("Already downloaded: skipping")
    return

  r = requests.get(url, stream=True)
  r.raise_for_status()
  with open(file_name, 'wb') as f:
    for chunk in r.iter_content(chunk_size=4096):
      f.write(chunk)
  return url

urls = ["https://datasets.imdbws.com/name.basics.tsv.gz",
        "https://datasets.imdbws.com/title.principals.tsv.gz",
        "https://datasets.imdbws.com/title.basics.tsv.gz"]

for url in urls:
  download_url(url)

os.makedirs("data", exist_ok=True) # Generate (recursively) folders, ignores the comand if they already exists

print("Filtering actors...")
df_attori = pd.read_csv(
  'name.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['nconst', 'primaryName', 'primaryProfession'], # Considering only this columns
  dtype={'primaryName': 'U', 'primaryProfession': 'U'}, # Both are unsigned integers
  converters={'nconst': lambda x: int(x.lstrip("nm0"))}) # All actors starts with nm0, we are just cleaning the output
df_attori.query('primaryProfession.str.contains("actor") or primaryProfession.str.contains("actress")', inplace=True)
# A lot of actors/actresses do more than one job (director etc..), with this comand I take all the names that have the string "actor" or "actress" in their profession. In this way both someone who is classified as "actor" or as "actor, director" are taken into consideration

print("Filtering films...")
df_film = pd.read_csv(
  'title.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'primaryTitle', 'isAdult', 'titleType'], # Considering only this columns
  dtype={'primaryTitle': 'U', 'titleType': 'U'}, # Both are unsigned integers
  converters={'tconst': lambda x: int(x.lstrip("t0")), 'isAdult': lambda x: x != "0"}) # # All movies starts with t0, we are just cleaning the output. Then remove all adult movies
df_film.query('not isAdult and titleType in ["movie", "tvSeries", "tvMovie", "tvMiniSeries"]',
              inplace=True) # There are a lot of junk categories considered in IMDb, we are considering all the non Adult movies in this whitelist
filtered_tconsts = df_film["tconst"].to_list()

print("Filtering relations...")
df_relazioni = pd.read_csv(
  'title.principals.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'nconst','category'],  # Considering only this columns
  dtype={'category': 'U'}, # Unsigned integer
  converters={'nconst': lambda x: int(x.lstrip("nm0")), 'tconst': lambda x: int(x.lstrip("t0"))}) # Cleaning
df_relazioni.query('(category == "actor" or category == "actress") and tconst in @filtered_tconsts', inplace=True)
# Returns an array of unique actor ids (nconsts) and an array of how many times they appear (counts) => the number of movies they appear in
nconsts, counts = np.unique(df_relazioni["nconst"].to_numpy(), return_counts=True)
filtered_nconsts = nconsts[counts>=MIN_MOVIES]
df_relazioni.query("nconst in @filtered_nconsts", inplace=True)

# Now select only films and actors that have at lest a relation
print("Re-filtering actors...")
nconsts_with_relations = df_relazioni["nconst"].unique()
df_attori.query("nconst in @nconsts_with_relations", inplace=True)
print("Re-filtering films...")
tconsts_with_relations = df_relazioni["tconst"].unique()
df_film.query("tconst in @tconsts_with_relations", inplace=True)

# Write the filtered files
df_attori.to_csv('data/Attori.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['nconst', 'primaryName'], header=False, index=False)
df_film.to_csv('data/FilmFiltrati.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['tconst', 'primaryTitle'], header=False, index=False)
df_relazioni.to_csv('data/Relazioni.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['tconst', 'nconst'], header=False, index=False)

# Takes about 1 min 30 s with MIN_MOVIES = 42
