import requests
from multiprocessing.pool import ThreadPool
import gzip
import pandas as pd
# import os
 
def download_url(url):
  print("downloading: ",url)
  file_name_start_pos = url.rfind("/") + 1
  file_name = url[file_name_start_pos:]
 
  r = requests.get(url, stream=True)
  if r.status_code == requests.codes.ok:
    with open(file_name, 'wb') as f:
      for data in r:
        f.write(data)
  return url

urls = ["https://datasets.imdbws.com/name.basics.tsv.gz", 
        "https://datasets.imdbws.com/title.principals.tsv.gz",
        "https://datasets.imdbws.com/title.basics.tsv.gz"]

# Run 3 multiple threads. Each call will take the next element in urls list
results = ThreadPool(3).imap_unordered(download_url, urls)
for r in results:
    print(r)

def titlebasics(): 
    df = pd.read_csv('title.basics.tsv.gz', sep='\t', usecols=['tconst', 'primaryTitle', 'isAdult'], compression='gzip')
    df.query('isAdult != 1', inplace=True)
    df.to_csv('FilmFiltrati.txt', sep=' ', columns=['tconst', 'primaryTitle'], header=False)

def namebasics():
    df = pd.read_csv('name.basics.tsv.gz', sep='\t', usecols=['nconst', 'primaryName', 'primaryProfession'], compression='gzip')
    df.query('primaryProfession == "actor" or primaryProfession == "actress"', inplace=True)
    df.to_csv('Attori.txt', sep=' ', columns=['nconst', 'primaryName'], header=False)

def titleprincipals():
    df = pd.read_csv('title.principals.tsv.gz', sep='\t', usecols=['nconst','category'], compression='gzip')
    df.query('category == "actor" or category == "actress"', inplace=True)    
    df.to_csv('') #DA FARE



titlebasics()
namebasics()
# titleprincipals()



# def cancella():
#     os.system('rm *.gz')





        







