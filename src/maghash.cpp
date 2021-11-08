#include <iostream>
#include <vector>
#include <string>
#include <stdlib.h>
#include <time.h>
using namespace std;

/*

https://www.usenix.org/system/files/conference/nsdi16/nsdi16-paper-eisenbud.pdf

*/

// number of backends
int N = 0;

// lookup table size. 
// should be s.t M >> N
int M = 0;

// we need to hash the backend name using two different
// has functions. per stack overflow, murmur and jenkins 
// seem to do well. 
uint32_t murmur_OOAT_32(const string backend)
{
    const char* str = backend.c_str();
    uint32_t h = 0;
    // iniitalize random seed
    srand(time(NULL));

    h = rand();
 
    // One-byte-at-a-time hash based on Murmur's mix
    // Source: https://github.com/aappleby/smhasher/blob/master/src/Hashes.cpp
    for (; *str; ++str) {
        h ^= *str;
        h *= 0x5bd1e995;
        h ^= h >> 15;
    }
    return h;
}

uint32_t jenkins_one_at_a_time_hash(const string& backend)
{
    size_t i = 0;
    uint32_t hash = 0;
    size_t length = backend.length();

    while (i != length) {
        hash += backend[i++];
        hash += hash << 10;
        hash ^= hash >> 6;
    }
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;
    return hash;
}

void compute_backend_preference(const vector<string>& backend_names, vector<vector<int>>& preferences)
/*
for a given backend, say i-th backend, preferences[i] will have a random array of values
the randomness is determined by the hash functions specific to maglev.
the property provided by the hash function is s.t upon addition or removal of a backend 
the remaining backends are assigned the same lookup slots.
*/
{
    int i = 0;
    for(auto backend: backend_names)
    {   
        // fill preferences[i] with the random values based on backend name        
        uint32_t offset = murmur_OOAT_32(backend) % M;
        uint32_t skip  = 1 + jenkins_one_at_a_time_hash(backend) % (M-1);

        for (uint32_t j = 0; j < M; j++)
        {
            preferences[i][j] = (offset + j*skip) % M;
        }

        i++;
    }
    return;
}

void print_mux_assignment(const vector<int>& entry, const vector<string>& backend_names)
{
    cout << "Which backend gets what entry in the lookup table? "<<endl;
    for (int i = 0; i < entry.size(); i++)
    {
        cout << i << " | " << backend_names[entry[i]]<<endl;
    }

    vector<int> mux_distribution(backend_names.size(), 0);
    for (int i =0; i < entry.size(); i++)
    {
        mux_distribution[entry[i]]++;
    }

    cout << "Backend load displays how many entries in the lookup are per backend." <<endl;
    for (int i = 0; i < mux_distribution.size(); i++)
    {
        cout << backend_names[i] << " :: " << mux_distribution[i]<<endl;
    }

}


void maglevHash(const vector<string>& backend_names)
{
    N = backend_names.size();
    // M >> N per the maglev paper
    M = 113;

    vector<int> next(N, 0);
    vector<int> entry(M, -1);

    // preferences is for each of the N backend, we compute
    // randomly generated preferences for M lookup positions
    vector<vector<int>> preferences(N);
    for(auto it = preferences.begin(); it != preferences.end(); it++)
    {
       (*it).assign(M, 0);
    }

    compute_backend_preference(backend_names, preferences);

    int n = 0;
    while (true)
    {
        for (int i = 0; i < N; i++)
        {
            int c = preferences[i][next[i]];

            // for this backend, find an unassigned lookup
            // table.
            while (entry[c] >= 0)
            {
                // this entry is taken. lets try the next preference.
                next[i]++;
                c = preferences[i][next[i]];
            }

            // assign backend to this entry.
            entry[c] = i;
            next[i]++;
            n++;

            if (n == M) 
            {
                goto done;
            }                
        }
    }

done:    
    print_mux_assignment(entry, backend_names);

    return;
}


int main(int argc, char** argv)
{
    cout<< "Sample hashing used by maglev slb"<<endl;

    vector<string> backend_names1 { "dip1", "dip2", "dip3", "dip4", "dip5" };
    vector<string> backend_names2 {"dip1", "dip2", "dip3", "dip4", "dip6"};

    maglevHash(backend_names1);
    maglevHash(backend_names2);
    return 0;    
}