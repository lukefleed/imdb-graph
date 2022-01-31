// g++ -Wall -pedantic -std=c++17 -pthread kenobi.cpp -o kenobi
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <list>
#include <stack>
#include <set>
#include <thread>
#include <fstream> // getline
#include <algorithm> // find
#include <math.h> // ceil
#include <sys/time.h> // per gettimeofday

using namespace std;

struct Film {
    string name;
    vector<int> actor_indicies;
};

struct Actor {
    string name;
    vector<int> film_indices;
};

map<int, Actor> A; // Dizionario {actor_id (key): Actor (value)}
map<int, Film> F; // Dizionario {film_id (value): Film (value)}
int MAX_ACTOR_ID = -1;

void DataRead()
{
    ifstream actors("data/Attori.txt"); // leggo il file
    ifstream movies("data/FilmFiltrati.txt"); // leggo il file

    string s,t; // creo delle stringhe per dopo, notazione triste? Si
    const string space /* the final frontier */ = " "; // stringa spazio per dopo

    for (int i = 1; getline(actors,s); i++)
    {
        if (s.empty()) // serve per saltare le righe vuote, a volte capita
            continue;
        try {
            Actor TmpObj; // creo un oggetto temporaneo della classe Actor
            int id = stoi(s.substr(0, s.find(space)));
            TmpObj.name = s.substr(s.find(space)+1);
            A[id] = TmpObj; // Notazione di Matlab/Python, ma da C++17 funziona
            if (id > MAX_ACTOR_ID)
                MAX_ACTOR_ID = id;
        } catch (...) {
            cout << "Could not read the line " << i << " of Actors file" << endl;
        }
    }

    for (int i = 1; getline(movies,t); i++)
    {
        if (t.empty())
            continue;

        try{
            Film TmpObj;
            int id = stoi(t.substr(0, t.find(space)));
            TmpObj.name = t.substr(t.find(space)+1);
            F[id] = TmpObj;
        } catch (...) {
            cout << "Could not read the line " << i << " of Film file" << endl;
        }
    }
}

// Inizio a costruire il grafo

void BuildGraph()
{
    ifstream relations("data/Relazioni.txt");
    string s;
    const string space = " ";

    for (int i=1; getline(relations,s); i++){ // Scorro relations
        if (s.empty())
            continue;
        try {
            int id_film = stoi(s.substr(0, s.find(space))); // Indice del film
            int id_attore = stoi(s.substr(s.find(space)+1)); // Indice dell'attore
            if (A.count(id_attore) && F.count(id_film)) { // Escludi film e attori filtrati
                A[id_attore].film_indices.push_back(id_film);
                F[id_film].actor_indicies.push_back(id_attore);
            }
        } catch (...) {
            cout << "Could not read the line " << i << " of Releations file" << endl;
        }
    }
}

// Stampo il grafo (i primi max_n_film soltanto)
void PrintGraph(size_t max_n_film = 100)
{
    const size_t n = min(max_n_film, F.size()); // Potrebbero esserci meno film di max_n_film
    size_t i = 0;
    for (const auto& [id_film, film] : F) { // Loop sulle coppie id:film della mappa
        cout << id_film << "(" << film.name << ")";
        if (!film.actor_indicies.empty()) {
            cout << ":";
            for (int id_attore : film.actor_indicies)
                cout << " " << id_attore << "(" << A[id_attore].name << ")";
        }
        cout << endl;

        i++; // Tengo il conto di quanti ne ho stampati
        if (i >= n) // e smetto quando arrivo ad n
            break;
    }
}

// Trova un film in base al titolo, restituisce -1 se non lo trova
int FindFilm(string title)
{
    for (const auto& [id, film] : F)
        if (film.name == title)
            return id;
    return -1;
}

// Trova un film in base al titolo, restituisce -1 se non lo trova
int FindActor(string name)
{
    for (const auto& [id, actor] : A)
        if (actor.name == name)
            return id;
    return -1;
}
