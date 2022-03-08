void BuildGraph()
{
    ifstream relations("data/Relazioni.txt");
    string s;
    const string space = "\t";

    for (int i=1; getline(relations,s); i++){
        if (s.empty())
            continue;
        try {
            int id_film = stoi(s.substr(0, s.find(space)));
            int id_attore = stoi(s.substr(s.find(space)+1));
            if (A.count(id_attore) && F.count(id_film)) { // Exclude movies and actors filtered
                A[id_attore].film_indices.push_back(id_film);
                F[id_film].actor_indicies.push_back(id_attore);
            }
        } catch (...) {
            cout << "Could not read the line " << i << " of Releations file" << endl;
        }
    }
}
