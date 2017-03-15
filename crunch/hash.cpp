#include "hash.hpp"
#include <fstream>
#include <vector>
#include <iostream>
#include <sstream>
#include "tinydir.h"
#include "str.hpp"

template <class T>
void hashCombine(std::size_t& hash, const T& v)
{
    std::hash<T> hasher;
    hash ^= hasher(v) + 0x9e3779b9 + (hash<<6) + (hash>>2);
}

void hashFile(size_t& hash, const string& file)
{
    ifstream stream(file, ios::binary | ios::ate);
    streamsize size = stream.tellg();
    stream.seekg(0, ios::beg);
    vector<char> buffer(size + 1);
    if (!stream.read(buffer.data(), size))
    {
        cerr << "failed to read file: " << file << endl;
        exit(EXIT_FAILURE);
    }
    string text(buffer.begin(), buffer.end());
    hashCombine(hash, text);
}

void hashFiles(size_t& hash, const string& root)
{
    static string dot1 = ".";
    static string dot2 = "..";
    
    tinydir_dir dir;
    tinydir_open(&dir, StrToFile(root.data()).data());
    
    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        
        if (file.is_dir)
        {
            if (dot1 != FileToStr(file.name) && dot2 != FileToStr(file.name))
                hashFiles(hash, FileToStr(file.path));
        }
        else if (FileToStr(file.extension) == "png")
            hashFile(hash, FileToStr(file.path));
        
        tinydir_next(&dir);
    }
    
    tinydir_close(&dir);
}

bool loadHash(size_t& hash, const string& file)
{
    ifstream stream(file);
    if (stream)
    {
        stringstream ss;
        ss << stream.rdbuf();
        ss >> hash;
        return true;
    }
    return false;
}

void saveHash(size_t hash, const string& file)
{
    ofstream stream(file);
    stream << hash;
}
