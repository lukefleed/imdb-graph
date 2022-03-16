void DataRead()
{
    ifstream actors("data/Attori.txt");
    ifstream movies("data/FilmFiltrati.txt");

    string s,t;
    const string space /* the final frontier */ = "\t";
    for (int i = 1; getline(actors,s); i++)
    {
        if (s.empty())
            continue;
        try {
            Actor TmpObj;
            int id = stoi(s.substr(0, s.find(space)));
            TmpObj.name = s.substr(s.find(space)+1);
            A[id] = TmpObj; // Python notation, works with C++17
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
