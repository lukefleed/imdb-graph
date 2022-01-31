#!/usr/bin/env python3
import requests
import pandas as pd
import os
import csv

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

os.makedirs("data", exist_ok=True)

print("Filtering actors...")
df_attori = pd.read_csv(
  'name.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['nconst', 'primaryName', 'primaryProfession'],
  dtype={'primaryName': 'U', 'primaryProfession': 'U'},
  converters={'nconst': lambda x: int(x.lstrip("nm0"))})
df_attori.query('primaryProfession.str.contains("actor") or primaryProfession.str.contains("actress")', inplace=True)
df_attori.to_csv('data/Attori.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['nconst', 'primaryName'], header=False, index=False)
del df_attori  # Free memory

print("Filtering films...")
df_film = pd.read_csv(
  'title.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'primaryTitle', 'isAdult', 'titleType'],
  dtype={'primaryTitle': 'U', 'titleType': 'U'},
  converters={'tconst': lambda x: int(x.lstrip("t0")), 'isAdult': lambda x: x != "0"})
df_film.query('not isAdult and titleType in ["movie", "tvSeries", "tvMovie", "tvMiniSeries"]',
              inplace=True)
df_film.to_csv('data/FilmFiltrati.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['tconst', 'primaryTitle'], header=False, index=False)
filtered_tconsts = df_film["tconst"].to_list()
del df_film  # Free memory

print("Filtering relations...")
df_relazioni = pd.read_csv(
  'title.principals.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'nconst','category'],
  dtype={'category': 'U'},
  converters={'nconst': lambda x: int(x.lstrip("nm0")), 'tconst': lambda x: int(x.lstrip("t0"))})
df_relazioni.query('(category == "actor" or category == "actress") and tconst in @filtered_tconsts', inplace=True)
df_relazioni.to_csv('data/Relazioni.txt', sep='\t', quoting=csv.QUOTE_NONE, escapechar='\\', columns=['tconst', 'nconst'], header=False, index=False)
