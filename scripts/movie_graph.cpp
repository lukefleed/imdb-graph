// g++ -Wall -pedantic -std=c++17 -Ofast -pthread movie_graph.cpp -o movie_graph
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
map<int, Film> F; // Dictionary {film_id (key): Film (value)}
int MAX_MOVIE_ID = -1; // Here DataRead() puts the larges actor_id loaded from Attori.txt

const int N_THREADS = 12; // Number of threads to use for some functions

void DataRead()
{
    ifstream actors("../data/data_movie_graph/Attori.txt"); // read the file
    ifstream movies("../data/data_movie_graph/FilmFiltrati.txt"); // read the file

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
            if (id > MAX_MOVIE_ID)
                MAX_MOVIE_ID = id;
        } catch (...) {
            cout << "Could not read the line " << i << " of Film file" << endl;
        }
    }
}

void BuildGraph()
{
    ifstream relations("../data/data_movie_graph/Relazioni.txt");
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

void PrintGraph(size_t max_n_movie = 200)
{
    const size_t n = min(max_n_movie, F.size()); // There could be less film than max actors!
    size_t i = 0;
    for (const auto& [id_film, film] : F) {
        cout << id_film << " (" << film.name << ")";
        if (!film.actor_indicies.empty()) {
            cout << ":\n";
            for (int id_attore : film.actor_indicies) {
                cout << "\t- " << id_attore << " (" << A[id_attore].name << ")\n";
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

    vector<pair<int, double>> top_movies; // Each pair is (movie_index, farness).
    top_movies.reserve(k+1);  // We need exactly k items, no more and no less.

    vector<thread> threads;
    mutex top_movies_mutex; // The threads write to top_movies, so another thread reading top_movies at the same time may find it in an invalid state (if the read happens while the other thread is still writing)
    threads.reserve(N_THREADS);
    for (int i = 0; i < N_THREADS; i++) {
        // Launching the threads
        threads.push_back(thread([&top_movies,&top_movies_mutex,&k](int start) {
            vector<bool> enqueued(MAX_MOVIE_ID, false); // Vector to see which vertices with put in the queue during the BSF
            // We loop over each vertex
            for (int film_id = start; film_id <= MAX_MOVIE_ID; film_id += N_THREADS) {
                if (!F.count(film_id)) // The movie must exist, otherwise F[film_id] would attempt to write F, and this may produce a race condition if multiple threads do it at the same time
                    continue;

                // We just compute the farness of our vertex using a BFS
                queue<pair<int,int>> q; // FIFO of pairs (film_index, distance from our vertex).
                for (size_t i = 0; i < enqueued.size(); i++)
                    enqueued[i] = false;
                int r = 0; // |R|, where R is the set of vertices reachable from our vertex
                long long int sum_distances = 0; // Sum of the distances to other nodes
                int prev_distance = 0; // Previous distance, to see when we get to a deeper level of the BFS
                q.push(make_pair(film_id, 0)); // This vertex, which is at distance 0
                enqueued[film_id] = true;
                bool skip = false;
                while (!q.empty()) {
                    auto [bfs_film_id, distance] = q.front(); // Prendo l'elemento in cima alla coda
                    q.pop();
                    // Try to set a lower bound on the farness
                    if (distance > prev_distance) {
                        top_movies_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                        if (top_movies.size() == k) { // We are in the first item of the next exploration level
                            // We assume r = A.size(), the maximum possible value
                            double farness_lower_bound = 1.0 / ((double)F.size() - 1) * (sum_distances + q.size() * distance);
                            //cout << "LB: \x1b[36m" << farness_lower_bound << "\x1b[0m" << endl;
                            if (top_movies[k-1].second <= farness_lower_bound) { // Stop the BFS
                                skip = true;
                                top_movies_mutex.unlock(); // Release the ownership
                                break;
                            }
                        }
                        top_movies_mutex.unlock(); // Release the ownership
                    }
                    // We compute the farness of our vertex actor_id
                    r++;
                    sum_distances += distance;
                    // We loop on each actor on each film that bfs_actor_id played in, and add them to the queue
                    for (int bfs_actor_id : F[bfs_film_id].actor_indicies) {
                        for (int adj_film_id : A[bfs_actor_id].film_indices) {
                            if (!enqueued[adj_film_id]) {
                                // The adjacent vertices have distance +1 with respect to the current vertex
                                q.push(make_pair(adj_film_id, distance+1));
                                enqueued[adj_film_id] = true;
                            }
                        }
                    }
                }
                if (skip) {
                    cout << film_id << " " << F[film_id].name << " SKIPPED" << endl;
                    continue;
                }
                // BFS is over, we compute the farness
                double farness;
                if (r <= 1) // Avoid computing something/0
                    farness = numeric_limits<double>::infinity();
                else
                    farness = (double)(F.size()-1) / pow((double)r-1, 2) * (double)sum_distances;

                top_movies_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                // Insert the actor in top_movies, before the first element with farness >= than our actor's (i.e. sorted insertion)
                auto index = find_if(top_movies.begin(), top_movies.end(),
                                [&farness](const pair<int, double>& p) { return p.second > farness; });
                top_movies.insert(index, make_pair(film_id, farness));
                if (top_movies.size() > k)
                    top_movies.pop_back();
                top_movies_mutex.unlock(); // Release the ownerhsip (we are done with top_movies)

                cout << film_id << " " << F[film_id].name << "\n\tCC: " << 1.0/farness << endl;
                // top_actors_lock gets destroyed after this line, releasing the mutex
            }
        }, i));
    }

    for (auto& thread : threads)
        // Waiting for all threads to finish
        thread.join();

    ofstream output_file("../visualization/movie_graph/data/top_movies_c.txt");
    for (const auto& [film_id, farness] : top_movies) {
        output_file << film_id << "\t" << F[film_id].name << "\t" << 1.0/farness << endl;
    }

    return top_movies;

}

vector<pair<int, double>> harmonic(const size_t k) { //

    vector<pair<int, double>> top_movies; // Each pair is (actor_index, harmonic centrality).
    top_movies.reserve(k+1);  // We need exactly k items, no more and no less.

    vector<thread> threads;
    mutex top_movies_mutex; // To prevent simultaneous accesses to top_movies
    threads.reserve(N_THREADS);
    for (int i = 0; i < N_THREADS; i++) {
        threads.push_back(thread([&top_movies,&top_movies_mutex,&k](int start) {
            vector<bool> enqueued(MAX_MOVIE_ID, false); // Vector to see which vertices with put in the queue during the BSF
            // We loop over each vertex
            for (int film_id = start; film_id <= MAX_MOVIE_ID; film_id += N_THREADS) {
                if (!F.count(film_id)) // The actor must exist, otherwise A[actor_id] would attempt to write A, and this may produce a race condition if multiple threads do it at the same time
                    continue;
                // if |Top| ≥ k and L[v] > Farn[Top[k]] then return Top; => We can not exploit the lower bound of our vertex to stop the loop, as we are not updating lower bounds L.
                // We just compute the farness of our vertex using a BFS
                queue<pair<int,int>> q; // FIFO of pairs (actor_index, distance from our vertex).
                for (size_t i = 0; i < enqueued.size(); i++)
                    enqueued[i] = false;
                int r = 0; // |R|, where R is the set of vertices reachable from our vertex
                double sum_reverse_distances = 0; // Sum of the distances to other nodes
                int prev_distance = 0; // Previous distance, to see when we get to a deeper level of the BFS
                q.push(make_pair(film_id, 0));
                enqueued[film_id] = true;
                bool skip = false;
                while (!q.empty()) {
                    auto [bfs_film_id, distance] = q.front();
                    q.pop();
                    // Try to set an upper bound on the centrality
                    if (distance > prev_distance) {
                        top_movies_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                        if (top_movies.size() == k) { // We are in the first item of the next exploration level
                            double harmonic_centrality_upper_bound = sum_reverse_distances + q.size() / (double)distance + (F.size() - r - q.size()) / (double)(distance + 1);
                            if (top_movies[k-1].second >= harmonic_centrality_upper_bound) { // Stop the BFS
                                skip = true;
                                top_movies_mutex.unlock(); // Release the ownership
                                break;
                            }
                        }
                        top_movies_mutex.unlock(); // Release the ownership
                    }
                    // We compute the farness of our vertex actor_id
                    r++;
                    if (distance != 0)
                        sum_reverse_distances += 1.0/distance;
                    // We loop on the adjacencies of bfs_actor_id and add them to the queue
                    for (int bfs_actor_id : F[bfs_film_id].actor_indicies) {
                        for (int adj_film_id : A[bfs_actor_id].film_indices) {
                            if (!enqueued[adj_film_id]) {
                                // The adjacent vertices have distance +1 with respect to the current vertex
                                q.push(make_pair(adj_film_id, distance+1));
                                enqueued[adj_film_id] = true;
                            }
                        }
                    }
                }
                if (skip) {
                    cout << film_id << " " << F[film_id].name << " SKIPPED" << endl;
                    continue;
                }
                // BFS is over, we compute the centrality
                double harmonic_centrality = sum_reverse_distances;
                if (!isfinite(harmonic_centrality))
                    continue;

                top_movies_mutex.lock(); // Acquire ownership of the mutex, wait if another thread already owns it
                // Insert the actor in top_movies, before the first element with farness >= than our actor's (i.e. sorted insertion)
                auto index = find_if(top_movies.begin(), top_movies.end(),
                                [&harmonic_centrality](const pair<int, double>& p) { return p.second < harmonic_centrality; });
                top_movies.insert(index, make_pair(film_id, harmonic_centrality));
                if (top_movies.size() > k)
                    top_movies.pop_back();
                cout << film_id << " " << F[film_id].name << "\n\tHC: " << harmonic_centrality << endl;
                top_movies_mutex.unlock(); // Release the ownership
            }
        }, i));
    }

    for (auto& thread : threads)
        thread.join();

    ofstream output_file("../visualization/movie_graph/data/top_movies_h.txt");
    for (const auto& [film_id, harmonic] : top_movies) {
        output_file << film_id << "\t" << F[film_id].name << "\t" << harmonic << endl;
    }


    return top_movies;
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
        const auto& [closeness_film_id, farness] = top_by_closeness[i];
        const auto& [centrality_film_id, centrality] = top_by_harmonic[i];
        printf("%25s : %8lg        %25s : %8lg\n",
               F[closeness_film_id].name.c_str(), 1.0/farness,
               F[centrality_film_id].name.c_str(), centrality);
    }
    // for (const auto& [actor_id, farness] : top_by_closeness) {
    //         cout << A[actor_id].name << "\n\tCloseness Centrality: " << 1.0/farness << endl;
    // }
    // for (const auto& [actor_id, centrality] : top_by_harmonic) {
    //         cout << A[actor_id].name << "\n\tHarmonic Centrality: " << centrality << endl;
    // }
}
