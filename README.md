# IMDb Graph - Documentation

Introduction **TODO**

## Understanding the data

We are taking the data from the official IMDB dataset: https://datasets.imdbws.com/

In particolar we're interest in 3 files

- `title.basics.tsv`
- `title.principals.tsv`
- `name.basics.tsv`

Let's have a closer look to this 3 files:

### title.basics.tsv.gz
_Contains the following information for titles:_
* **tconst** (string) - alphanumeric unique identifier of the title
* **titleType** (string) – the type/format of the title (e.g. movie, short, tvseries, tvepisode, video, etc)
* **primaryTitle** (string) – the more popular title / the title used by the filmmakers on promotional materials at the point of release
* **originalTitle** (string) - original title, in the original language
* **isAdult** (boolean) - 0: non-adult title; 1: adult title
* **startYear** (YYYY) – represents the release year of a title. In the case of TV Series, it is the series start year
* **endYear** (YYYY) – TV Series end year. ‘\N’ for all other title types
* **runtimeMinutes** – primary runtime of the title, in minutes
* **genres** (string array) – includes up to three genres associated with the title

### title.principals.tsv.gz
_Contains the principal cast/crew for titles_
* **tconst** (string) - alphanumeric unique identifier of the title
* **ordering** (integer) – a number to uniquely identify rows for a given titleId
* **nconst** (string) - alphanumeric unique identifier of the name/person
* **category** (string) - the category of job that person was in
* **job** (string) - the specific job title if applicable, else '\N'
* **characters** (string) - the name of the character played if applicable, else '\N'

### name.basics.tsv.gz
_Contains the following information for names:_
* **nconst** (string) - alphanumeric unique identifier of the name/person
* **primaryName** (string)– name by which the person is most often credited
* **birthYear** – in YYYY format
* **deathYear** – in YYYY format if applicable, else '\N'
* **primaryProfession** (array of strings)– the top-3 professions of the person
* **knownForTitles** (array of tconsts) – titles the person is known for



## Filtering

> All This section refers to what's inside the file [filtro.py](https://github.com/lukefleed/imdb-graph/blob/main/filtro.py)

Now that we have downloaded all the files from the dataset, we have to filter them and modify them in order to easly work with them.

### name.basics.tsv

For this file we only need the following columns

- `nconst`
- `primaryTitle`
- `primaryProfession`

Since all the actors starts with the string `nm0` we can remove it to clean the output. Furthermore a lot of actors/actresses do more than one job (director etc..), to avoid excluding important actors we consider all the one that have the string `actor/actress` in their profession. In this way, both someone who is classified as `actor` or as `actor, director` are taken into consideration

```python
df_attori = pd.read_csv(
  'name.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['nconst', 'primaryName', 'primaryProfession'],
  dtype={'primaryName': 'U', 'primaryProfession': 'U'},
  converters={'nconst': lambda x: int(x.lstrip("nm0"))})
df_attori.query('primaryProfession.str.contains("actor") or primaryProfession.str.contains("actress")', inplace=True)
```
Then we can generate the final filtered file `Attori.txt` that has only two columns: `nconst` and `primaryName`

---

### title.basics.tsv.gz

For this file we only need the following columns

- `tconst`
- `primaryTitle`
- `isAdult`
- `titleType`

Since all the movies starts with the string `t0` we can remove it to clean the output. In this case, we also want to remove all the movies for adults.

There are a lot of junk categories considered in IMDb, we are considering all the non adult movies in this whitelist

- `movie`
- `tvSeries`
- `tvMovie`
- `tvMiniSeries`

Why this in particolar? Benefits on the computational cost. There are (really) a lot of single episodes listed in IMDb: to remove them without loosing the most important relations, we only consider the category `tvSeries`. This category list a TV-Series as a single element, not divided in multiple episodes. In this way we will loose some of the relations with minor actors that may appears in just a few episodes. But we will have preserved the relations between the protagonist of the show. _It's not much, but it's an honest work_

```python
print("Filtering films...")
df_film = pd.read_csv(
  'title.basics.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'primaryTitle', 'isAdult', 'titleType'],
  dtype={'primaryTitle': 'U', 'titleType': 'U'},
  converters={'tconst': lambda x: int(x.lstrip("t0")), 'isAdult': lambda x: x != "0"}) #
df_film.query('not isAdult and titleType in ["movie", "tvSeries", "tvMovie", "tvMiniSeries"]',
              inplace=True)
filtered_tconsts = df_film["tconst"].to_list()
```


Then we can generate the final filtered file `FilmFiltrati.txt` that has only two columns: `nconst` and `primaryName`

---

### title.principals.tsv

For this file we only need the following columns

- `tconst`
- `nconst`
- `category`

As before, we clean the output removing unnecessary strings. Then we create an array on unique actor ids (`nconst`) and an array of how may times they appear (`counts`). This will give us the number of movies they appear in. And here it comes the core of this filtering. We define at the start of the algorithm a constant `MIN_MOVIES`. This integer is the minimum number of movies that an actor has to have done in his carrier to be considered in this graph. The reason to do that it's purely computational. If I have to consider all actors the time for the code to compile is the _year(s)'s_ order, that's not good. We are making an approximation: if an actor has less then a reasonable (_42_, as an example) number of movies made in his carrier, there is an high probability that he/she has an important role in our graph during the computation of the centralities.

```python
print("Filtering relations...")
df_relazioni = pd.read_csv(
  'title.principals.tsv.gz', sep='\t', compression='gzip',
  usecols=['tconst', 'nconst','category'],
  dtype={'category': 'U'},
  converters={'nconst': lambda x: int(x.lstrip("nm0")), 'tconst': lambda x: int(x.lstrip("t0"))})
df_relazioni.query('(category == "actor" or category == "actress") and tconst in @filtered_tconsts', inplace=True)
nconsts, counts = np.unique(df_relazioni["nconst"].to_numpy(), return_counts=True)
filtered_nconsts = nconsts[counts>=MIN_MOVIES]
df_relazioni.query("nconst in @filtered_nconsts", inplace=True)
```

Notice that we are only selecting actors and actresses that have at least a relation.

```python
print("Re-filtering actors...")
nconsts_with_relations = df_relazioni["nconst"].unique()
df_attori.query("nconst in @nconsts_with_relations", inplace=True)

print("Re-filtering films...")
tconsts_with_relations = df_relazioni["tconst"].unique()
df_film.query("tconst in @tconsts_with_relations", inplace=True)
```

At the end, we can finally generate the file `Relazioni.txt` containing the columns `tconst` and `nconst`

# Understanding the code
