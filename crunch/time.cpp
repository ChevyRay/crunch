#include "time.hpp"
#include <unordered_map>
#include <chrono>
#include <iostream>

// Uncomment line below if needed
// Note: measuring time is incompatible with g++
// #define MEASURE_TIME 

using namespace std;

static unordered_map<std::string, long long> funcs;
static unordered_map<std::string, chrono::steady_clock::time_point> starts;

void StartTimer(const string& func)
{
#ifdef MEASURE_TIME
    starts[func] = chrono::high_resolution_clock::now();
#endif
}

void StopTimer(const string& func)
{
#ifdef MEASURE_TIME
    auto now = chrono::high_resolution_clock::now();
    auto time = now - starts[func];
    funcs[func] += time.count();
#endif
}

void WriteAllTimers()
{
#ifdef MEASURE_TIME
    if(funcs["total"] == 0) StopTimer("total");
    cout << "Time measured:" << endl;
    for (auto &kv : funcs) 
        if(kv.first != "total" && !kv.first.ends_with('/'))
            cout << "\t" << kv.first << ": " << kv.second / 1000000.0 << " ms" << endl;
    
    for (auto &kv : funcs) 
        if(kv.first != "total" && kv.first.ends_with('/'))
            cout << "\tsubdir " << kv.first.substr(0, kv.first.length() - 1) << ": " << kv.second / 1000000.0 << " ms" << endl;
    
    cout << "\ttotal: " << funcs["total"] / 1000000.0 << " ms" << endl;
#endif
}
