// g++ -Wall -pedantic -std=c++17 -Ofast -pthread kenobi.cpp -o kenobi
#include <iostream>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <queue>
#include <list>
#include <thread>
#include <mutex>
#include <stack>
#include <set>
#include <fstream> // getline
#include <algorithm> // find
#include <math.h> // ceil
#include <sys/time.h>

using namespace std;

struct Film {
    string name;
    vector<int> actor_indicies;
};

struct Actor {
    string name;
    vector<int> film_indices;
};

map<int, Actor> A; // Dictionary {actor_id (key): Actor (value)}
map<int, Film> F; // Dictionary {film_id (value): Film (value)}
int MAX_ACTOR_ID = -1; // Here DataRead() puts the larges actor_id loaded from Attori.txt

const int N_THREADS = 12; // Number of threads to use for some functions

void DataRead()
{
    ifstream actors("../data/data_actor_graph/Attori.txt"); // read the file
    ifstream movies("../data/data_actor_graph/FilmFiltrati.txt"); // read the file

    string s,t;
    const string space /* the final frontier */ = "\t";

    for (int i = 1; getline(actors,s); i++)
    {
        if (s.empty()) // jumps empty lines, sometimes can happen
            continue;
        try {
            Actor TmpObj; // Temporary object for the actor class
            int id = stoi(s.substr(0, s.find(space)));
            TmpObj.name = s.substr(s.find(space)+1);
            A[id] = TmpObj; // Matlab/Python notation, works since C++17
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

void BuildGraph()
{
    ifstream relations("data/Relazioni.txt");
    string s;
    const string space = "\t";

    for (int i=1; getline(relations,s); i++){ // Scorro relations
        if (s.empty())
            continue;
        try {
            int id_film = stoi(s.substr(0, s.find(space))); // Index of the movie
            int id_attore = stoi(s.substr(s.find(space)+1)); // Index of the actor
            if (A.count(id_attore) && F.count(id_film)) { // Do not consider the filtered ones
                A[id_attore].film_indices.push_back(id_film);
                F[id_film].actor_indicies.push_back(id_attore);
            }
        } catch (...) {
            cout << "Could not read the line " << i << " of Releations file" << endl;
        }
    }
}

void PrintGraph(size_t max_n_actors = 3)
{
    const size_t n = min(max_n_actors, A.size()); // There could be less film than max actors!
    size_t i = 0;
    for (const auto& [id_attore, attore] : A) {
        cout << id_attore << " (" << attore.name << ")";
        if (!attore.film_indices.empty()) {
            cout << ":\n";
            for (int id_film : attore.film_indices) {
                cout << "\t- " << id_film << " (" << F[id_film].name << ")\n";
                for (int id_attore_adj : F[id_film].actor_indicies)
                    if (id_attore_adj != id_attore)
                        cout << "\t\t* " << id_attore_adj << " (" << A[id_attore_adj].name << ")\n";
            }
        }
        cout << endl;

        i++; // Taking count of how many are getting printed
        if (i >= n) // Stop when I arrive ad n
            break;
    }
}

// Find a movie by the title. Gives -1 if there is no match
int FindFilm(string title)
{
    for (const auto& [id, film] : F)
        if (film.name == title)
            return id;
    return -1;
}

// Find an actor by the name. Gives -1 if there is no match
int FindActor(string name)
{
    for (const auto& [id, actor] : A)
        if (actor.name == name)
            return id;
    return -1;
}

vector<pair<int, double>> closeness(const size_t k) {

    vector<pair<int, double>> top_actors; // Each pair is (actor_index, farness).
    top_actors.reserve(k+1);  // We need exactly k items, no more and no less.

    vector<thread> threads;
    mutex top_actors_mutex; // The threads write to top_actors, so another thread reading top_actors at the same time may find it in an invalid state (if the read happens while the other thread is still writing)
    threads.reserve(N_THREADS);
    for (int i = 0; i < N_THREADS; i++) {
        // Launching the threads
        threads.push_back(thread([&top_actors,&top_actors_mutex,&k](int start) {
            vector<bool> enqueued(MAX_ACTOR_ID, false); // Vector to see which vertices with put in the queue during the BSF
            // We loop over each vertex
            for (int actor_id = start; actor_id <= MAX_ACTOR_ID; actor_id += N_THREADS) {
                if (!A.count(actor_id)) // The actor must exist, otherwise A[actor_id] would attempt to write A, and this may produce a race condition if multiple threads do it at the same time
                    continue;

                // We just compute the farness of our vertex using a BFS
                queue<pair<int,int>> q; // FIFO of pairs (actor_index, distance from our vertex).
                for (size_t i = 0; i < enqueued.size(); i++)
                    enqueued[i] = false;
                int r = 0; // |R|, where R is the set of vertices reachable from our vertex
                long long int sum_distances = 0; // Sum of the distances to other nodes
                int prev_distance = 0; // Previous distance, to see when we get to a deeper level of the BFS
                q.push(make_pair(actor_id, 0)); // This vertex, which is at distance 0
                enqueued[actor_id] = true;
                bool skip = false;
                while (!q.empty()) {
                    auto [bfs_actor_id, distance] = q.front(); // Prendo l'elemento in cima alla coda
                    q.pop();
                    // Try to set a lower bound on the farness
                    if (distance > prev_distance) {
                        top_actors_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                        if (top_actors.size() == k) { // We are in the first item of the next exploration level
                            // We assume r = A.size(), the maximum possible value
                            double farness_lower_bound = 1.0 / ((double)A.size() - 1) * (sum_distances + q.size() * distance);
                            if (top_actors[k-1].second <= farness_lower_bound) { // Stop the BFS
                                skip = true;
                                top_actors_mutex.unlock(); // Release the ownership
                                break;
                            }
                        }
                        top_actors_mutex.unlock(); // Release the ownership
                    }
                    // We compute the farness of our vertex actor_id
                    r++;
                    sum_distances += distance;
                    // We loop on each actor on each film that bfs_actor_id played in, and add them to the queue
                    for (int bfs_film_id : A[bfs_actor_id].film_indices) {
                        for (int adj_actor_id : F[bfs_film_id].actor_indicies) {
                            if (!enqueued[adj_actor_id]) {
                                // The adjacent vertices have distance +1 with respect to the current vertex
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
                double farness;
                if (r <= 1) // Avoid computing something/0
                    farness = numeric_limits<double>::infinity();
                else
                    farness = (double)(A.size()-1) / pow((double)r-1, 2) * (double)sum_distances;

                top_actors_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                // Insert the actor in top_actors, before the first element with farness >= than our actor's (i.e. sorted insertion)
                auto index = find_if(top_actors.begin(), top_actors.end(),
                                [&farness](const pair<int, double>& p) { return p.second > farness; });
                top_actors.insert(index, make_pair(actor_id, farness));
                if (top_actors.size() > k)
                    top_actors.pop_back();
                top_actors_mutex.unlock(); // Release the ownerhsip (we are done with top_actors)

                cout << actor_id << " " << A[actor_id].name << "\n\tCC: " << 1.0/farness << endl;
                // top_actors_lock gets destroyed after this line, releasing the mutex
            }
        }, i));
    }

    for (auto& thread : threads)
        // Waiting for all threads to finish
        thread.join();

    ofstream output_file("visualization/top_actors_c.txt");
    for (const auto& [actor_id, farness] : top_actors) {
        output_file << actor_id << "\t" << A[actor_id].name << "\t" << 1.0/farness << endl;
    }

    return top_actors;

}

vector<pair<int, double>> harmonic(const size_t k) { //

    vector<pair<int, double>> top_actors; // Each pair is (actor_index, harmonic centrality).
    top_actors.reserve(k+1);  // We need exactly k items, no more and no less.

    vector<thread> threads;
    mutex top_actors_mutex; // To prevent simultaneous accesses to top_actors
    threads.reserve(N_THREADS);
    for (int i = 0; i < N_THREADS; i++) {
        threads.push_back(thread([&top_actors,&top_actors_mutex,&k](int start) {
            vector<bool> enqueued(MAX_ACTOR_ID, false); // Vector to see which vertices with put in the queue during the BSF
            // We loop over each vertex
            for (int actor_id = start; actor_id <= MAX_ACTOR_ID; actor_id += N_THREADS) {
                if (!A.count(actor_id)) // The actor must exist, otherwise A[actor_id] would attempt to write A, and this may produce a race condition if multiple threads do it at the same time
                    continue;
                // if |Top| ≥ k and L[v] > Farn[Top[k]] then return Top; => We can not exploit the lower bound of our vertex to stop the loop, as we are not updating lower bounds L.
                // We just compute the farness of our vertex using a BFS
                queue<pair<int,int>> q; // FIFO of pairs (actor_index, distance from our vertex).
                for (size_t i = 0; i < enqueued.size(); i++)
                    enqueued[i] = false;
                int r = 0; // |R|, where R is the set of vertices reachable from our vertex
                double sum_reverse_distances = 0; // Sum of the distances to other nodes
                int prev_distance = 0; // Previous distance, to see when we get to a deeper level of the BFS
                q.push(make_pair(actor_id, 0));
                enqueued[actor_id] = true;
                bool skip = false;
                while (!q.empty()) {
                    auto [bfs_actor_id, distance] = q.front();
                    q.pop();
                    // Try to set an upper bound on the centrality
                    if (distance > prev_distance) {
                        top_actors_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                        if (top_actors.size() == k) { // We are in the first item of the next exploration level
                            double harmonic_centrality_upper_bound = sum_reverse_distances + q.size() / (double)distance + (A.size() - r - q.size()) / (double)(distance + 1);
                            if (top_actors[k-1].second >= harmonic_centrality_upper_bound) { // Stop the BFS
                                skip = true;
                                top_actors_mutex.unlock(); // Release the ownership
                                break;
                            }
                        }
                        top_actors_mutex.unlock(); // Release the ownership
                    }
                    // We compute the farness of our vertex actor_id
                    r++;
                    if (distance != 0)
                        sum_reverse_distances += 1.0/distance;
                    // We loop on the adjacencies of bfs_actor_id and add them to the queue
                    for (int bfs_film_id : A[bfs_actor_id].film_indices) {
                        for (int adj_actor_id : F[bfs_film_id].actor_indicies) {
                            if (!enqueued[adj_actor_id]) {
                                // The adjacent vertices have distance +1 with respect to the current vertex
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
                // BFS is over, we compute the centrality
                double harmonic_centrality = sum_reverse_distances;
                if (!isfinite(harmonic_centrality))
                    continue;

                top_actors_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                // Insert the actor in top_actors, before the first element with farness >= than our actor's (i.e. sorted insertion)
                auto index = find_if(top_actors.begin(), top_actors.end(),
                                [&harmonic_centrality](const pair<int, double>& p) { return p.second < harmonic_centrality; });
                top_actors.insert(index, make_pair(actor_id, harmonic_centrality));
                if (top_actors.size() > k)
                    top_actors.pop_back();
                cout << actor_id << " " << A[actor_id].name << "\n\tHC: " << harmonic_centrality << endl;
                top_actors_mutex.unlock(); // Release the ownership
            }
        }, i));
    }

    for (auto& thread : threads)
        thread.join();

    ofstream output_file("visualization/top_actors_h.txt");
    for (const auto& [actor_id, harmonic] : top_actors) {
        output_file << actor_id << "\t" << A[actor_id].name << "\t" << harmonic << endl;
    }


    return top_actors;
}


int main()
{
    srand(time(NULL));

    DataRead();
    BuildGraph();
    cout << "Numero film: " << F.size() << endl;
    cout << "Numero attori: " << A.size() << endl;
    PrintGraph();

    // ------------------------------------------------------------- //

    // FUNZIONE CERCA FILM

    // cout << "Cerca film: ";
    // string titolo;
    // getline(cin, titolo);
    // int id_film = FindFilm(titolo);
    // cout << id_film << "(" << F[id_film].name << ")";
    // if (!F[id_film].actor_indicies.empty()) {
    //     cout << ":";
    //     for (int id_attore : F[id_film].actor_indicies)
    //         cout << " " << id_attore << "(" << A[id_attore].name << ")";
    // }
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
    //         cout << " " << id_attore << "(" << F[id_film].name << ")"; // Non worka ancora
    // }
    // cout << endl;

   // ------------------------------------------------------------- //

    cout << "Grafo, grafo delle mie brame... chi è il più centrale del reame?\n" <<endl;
    const size_t k = 100;
    auto top_by_closeness = closeness(k);
    auto top_by_harmonic = harmonic(k);
    printf("\n%36s        %36s\n", "CLOSENESS CENTRALITY", "HARMONIC CENTRALITY");
    for (size_t i = 0; i < k; i++) {
        const auto& [closeness_actor_id, farness] = top_by_closeness[i];
        const auto& [centrality_actor_id, centrality] = top_by_harmonic[i];
        printf("%25s : %8lg        %25s : %8lg\n",
               A[closeness_actor_id].name.c_str(), 1.0/farness,
               A[centrality_actor_id].name.c_str(), centrality);
    }
    // for (const auto& [actor_id, farness] : top_by_closeness) {
    //         cout << A[actor_id].name << "\n\tCloseness Centrality: " << 1.0/farness << endl;
    // }
    // for (const auto& [actor_id, centrality] : top_by_harmonic) {
    //         cout << A[actor_id].name << "\n\tHarmonic Centrality: " << centrality << endl;
    // }
}
