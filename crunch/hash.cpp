/*

 MIT License

 Copyright (c) 2017 Chevy Ray Johnston

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

 */

#include "hash.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>

using namespace std;
using namespace filesystem;

void HashCombine(uint64_t &hash, uint64_t v)
{
    hash ^= v + 0x9e3779b9 + (hash << 6) + (hash >> 2);
}

void HashData(uint64_t &hash, const char *data, uint64_t size)
{
    uint64_t seed = 131;
    uint64_t v = 0;
    for (int i = 0; i < size; ++i)
    {
        v = v * seed + data[i];
    }
    v &= 0x7fffffff;

    HashCombine(hash, v);
}

void HashString(uint64_t &hash, const string &str)
{
    HashData(hash, str.data(), str.size());
}

void HashFile(uint64_t &hash, const string &file, bool checkTime)
{
    if (checkTime)
    {
        auto time = last_write_time(file);
        HashCombine(hash, chrono::duration_cast<chrono::seconds>(time.time_since_epoch()).count());
        return;
    }
    ifstream stream(file, ios::binary | ios::ate);
    streamsize size = stream.tellg();
    stream.seekg(0, ios::beg);
    vector<char> buffer(size);
    if (!stream.read(buffer.data(), size))
    {
        cerr << "failed to read file: " << file << endl;
        exit(EXIT_FAILURE);
    }
    HashData(hash, buffer.data(), size);
}

static void HashFile(uint64_t &hash, const directory_entry &file, bool checkTime)
{
    if (checkTime)
    {
        auto time = file.last_write_time();
        HashCombine(hash, chrono::duration_cast<chrono::seconds>(time.time_since_epoch()).count());
    }
    else
        HashFile(hash, file.path().string(), false);
}

void HashFiles(uint64_t &hash, const string &root, bool checkTime)
{
    for (const auto &entry : directory_iterator(root))
    {
        if (entry.is_directory())
        {
            HashString(hash, entry.path().string());
            HashFiles(hash, entry.path().string(), checkTime);
        }
        else
            HashFile(hash, entry, checkTime);
    }
}

bool LoadHash(uint64_t &hash, const string &file)
{
    ifstream stream(file);
    if (!stream)
        return false;

    stream >> hash;
    return true;
}

void SaveHash(uint64_t hash, const string &file)
{
    ofstream stream(file);
    stream << hash << endl;
}
