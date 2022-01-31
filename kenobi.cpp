// g++ -Wall -pedantic -std=c++17 kenobi.cpp -o kenobi
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <list>
#include <stack>
#include <set>
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

vector<pair<int, double>> closeness(const size_t k) {
    /* **************************** ALGORITHM ****************************

    Input : A graph G = (V, E)
    Output: Top k nodes with highest closeness and their closeness values c(v)
        global L, Q ← computeBounds(G);
        global Top ← [ ];
        global Farn;

        for v ∈ V do Farn[v] = +∞;
        while Q is not empty do
            v ← Q.extractMin();
            if |Top| ≥ k and L[v] > Farn[Top[k]] then return Top;
            Farn[v] ← updateBounds(v); // This function might also modify L
            add v to Top, and sort Top according to Farn;
            update Q according to the new bounds;

    - We use a list TOP containing all “analysed” vertices v1 , . . . , vl in increasing order of farness

    - A priority queue Q containing all vertices “not analysed, yet”, in increasing order of lower bound L (this way, the head of Q always has the smallest value of L among all vertices in Q).

    - At the beginning, using the function computeBounds(), we compute a first bound L(v) for each vertex v, and we fill the queue Q according to this bound.

    - Then, at each step, we extract the first element v of Q: if L(v) is smaller than the k-th biggest farness computed until now (that is, the farness of the k-th vertex in variable Top), we can safely stop, because for each x ∈ Q, f (x) ≤ L(x) ≤ L(v) < f (Top[k]), and x is not in the top k.

    - Otherwise, we run the function updateBounds(v), which performs a BFS from v, returns the farness of v, and improves the bounds L of all other vertices. Finally, we insert v into Top in the right position, and we update Q if the lower bounds have changed.

    The crucial point of the algorithm is the definition of the lower bounds, that is, the
    definition of the functions computeBounds and updateBounds.

    Now let's define a conservative way (due to the fact that I only have a laptop and 16GB of RAM) to implement this two functions

    - computeBounds:
        The conservative strategy computeBoundsDeg needs time O(n): it simply sets L(v) = 0 for each v, and it fills Q by inserting nodes in decreasing order of degree (the idea is that vertices with high degree have small farness, and they should be analysed as early as possible, so that the values in TOP are correct as soon as possible). Note that the vertices can be sorted in time O(n) using counting sort.

    - updateBounds(w):
        the conservative strategy updateBoundsBFSCut(w) does not improve L, and it cuts the BFS as soon as it is sure that the farness of w is smaller than the k-th biggest farness found until now, that is, Farn[Top[k]]. If the BFS is cut, the function returns +∞, otherwise, at the end of the BFS we have computed the farness of v, and we can return it. The running time of this procedure is O(m) in the worst case, but
        it can be much better in practice. It remains to define how the procedure can be sure that the farness of v is at least x: to this purpose, during the BFS, we update a lower bound on the farness of v. The idea behind this bound is that, if we have already visited all nodes up to distance d, we can upper bound the closeness centrality of v by setting distance d + 1 to a number of vertices equal to the number of edges “leaving” level d, and distance d + 2 to all the remaining vertices.
    */
    // L = 0 for all vertices and is never update, so we do not need to define it. We will just loop over each vertex, in the order the map prefers.
    // We do not need to define Q either, as we will loop over each vertex anyway, and the order does not matter.
    vector<pair<int, double>> top_actors; // Each pair is (actor_index, farness).
    top_actors.reserve(k+1);  // We need exactly k items, no more and no less.
    vector<bool> enqueued(MAX_ACTOR_ID, false); // Vector to see which vertices with put in the queue during the BSF

    // We loop over each vertex
    for (const auto& [actor_id, actor] : A) {
        // if |Top| ≥ k and L[v] > Farn[Top[k]] then return Top; => We can not exploit the lower bound of our vertex to stop the loop, as we are not updating lower bounds L.
        // We just compute the farness of our vertex using a BFS
        queue<pair<int,int>> q; // FIFO of pairs (actor_index, distance from our vertex).
        for (size_t i = 0; i < enqueued.size(); i++)
            enqueued[i] = false;
        int r = 0; // |R|, where R is the set of vertices reachable from our vertex
        long long int sum_distances = 0; // Sum of the distances to other nodes
        int prev_distance = 0; // Previous distance, to see when we get to a deeper level of the BFS
        q.push(make_pair(actor_id, 0));
        enqueued[actor_id] = true;
        bool skip = false;
        while (!q.empty()) {
            auto [bfs_actor_id, distance] = q.front();
            q.pop();
            // Try to set a lower bound on the farness
            if (top_actors.size() == k && distance > prev_distance) { // We are in the first item of the next exploration level
                // We assume r = A.size(), the maximum possible value
                double farness_lower_bound = 1.0 / ((double)A.size() - 1) * (sum_distances + q.size() * distance);
                if (top_actors[k-1].second <= farness_lower_bound) { // Stop the BFS
                    skip = true;
                    break;
                }
            }
            // We compute the farness of our vertex actor_id
            r++;
            sum_distances += distance;
            // We loop on the adjacencies of bfs_actor_id and add them to the queue
            for (int bfs_film_id : A[bfs_actor_id].film_indices) {
                for (int adj_actor_id : F[bfs_film_id].actor_indicies) {
                    if (!enqueued[adj_actor_id]) {
                        // The adjacent vertices have distance +1 w.r.t. the current vertex
                        q.push(make_pair(adj_actor_id, distance+1));
                        enqueued[adj_actor_id] = true;
                    }
                }
            }
        }
        if (skip) {
            cout << actor_id << " " << A[actor_id].name << " SKIPPED" << endl;
            continue;
        }
        // BFS is over, we compute the farness
        double farness = (A.size()-1) / pow((double)r-1, 2) * sum_distances;
        if (isnan(farness)) // This happens when r = 1
            continue;
        // Insert the actor in top_actors, before the first element with farness >= than our actor's (i.e. sorted insert)
        auto idx = find_if(top_actors.begin(), top_actors.end(),
                           [&farness](const pair<int, double>& p) { return p.second >= farness; });
        if (top_actors.size() < k || idx != top_actors.end()) {
            top_actors.insert(idx, make_pair(actor_id, farness));
            if (top_actors.size() > k)
                top_actors.pop_back();
        }
        cout << actor_id << " " << A[actor_id].name << " " << farness << endl;
    }

    return top_actors;
}


int main()
{
    srand(time(NULL));

    // # info.txt valore massimo di un identificativo di un attore dentro Relazioni.txt, non so scriverlo in python quindi eccolo in bash
	// echo "$(cut -f2 -d' ' data/Relazioni.txt | sort --numeric-sort | tail -1)" > data/info.txt

    DataRead();
    BuildGraph();
    cout << "Numero film: " << F.size() << endl;
    cout << "Numero attori: " << A.size() << endl;
    PrintGraph();

    // ------------------------------------------------------------- //

    // // FUNZIONE CERCA FILMclos

    // cout << "Cerca film: ";
    // string titolo;
    // getline(cin, titolo);
    // int id_film = FindFilm(titolo);
    // cout << id_film << "(" << F[id_film].name << ")";
    // if (!F[id_film].actor_indicies.empty()) {
    //     cout << ":";
    //     for (int id_attore : F[id_film].actor_indicies)
    //         cout << " " << id_attore << "(" << A[id_attore].name << ")";
    // }clos
    // cout << endl;

    // // FUNZIONE CERCA ATTORE

    // cout << "Cerca attore: ";
    // string attore;
    // getline(cin, attore);
    // int id_attore = FindActor(attore);
    // cout << id_attore << "(" << A[id_attore].name << ")";
    // if (!A[id_attore].film_indices.empty()) {
    //     cout << ":";
    //     for (int id_attore : A[id_attore].film_indices)
    //         cout << " " << id_attore << "(" << A[id_attore].name << ")";
    // }
    // cout << endl;

    // ------------------------------------------------------------- //

    cout << "Grafo, grafo delle mie brame... chi è il più centrale del reame?" << endl;
    for (const auto& [actor_id, farness] : closeness(3)) {
        cout << A[actor_id].name << " " << farness << endl;
    }

}
