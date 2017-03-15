#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <sstream>
#include <vector>
#include "tinydir.h"
#include "bitmap.hpp"
#include "packer.hpp"

#define PACK_SIZE 1024

using namespace std;

static vector<Bitmap*> bitmaps;
static vector<Packer*> packers;

template <class T>
inline void hashCombine(std::size_t& hash, const T& v)
{
    std::hash<T> hasher;
    hash ^= hasher(v) + 0x9e3779b9 + (hash<<6) + (hash>>2);
}

static void hashFiles(size_t& hash, const string& root)
{
    static string dot1 = ".";
    static string dot2 = "..";
    
    tinydir_dir dir;
    tinydir_open(&dir, root.data());
    
    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        
        if (file.is_dir)
        {
            if (dot1 != file.name && dot2 != file.name)
                hashFiles(hash, file.path);
        }
        else if (strcmp(file.extension, "png") == 0)
        {
            ifstream stream(file.path, ios::binary | ios::ate);
            streamsize size = stream.tellg();
            stream.seekg(0, ios::beg);
            vector<char> buffer(size + 1);
            if (!stream.read(buffer.data(), size))
            {
                cerr << "failed to read file: " << file.path << endl;
                exit(EXIT_FAILURE);
            }
            string text(buffer.begin(), buffer.end());
            hashCombine(hash, text);
        }
        
        tinydir_next(&dir);
    }
    
    tinydir_close(&dir);
}

static bool loadHash(size_t& hash, const string& file)
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

static void saveHash(size_t hash, const string& file)
{
    ofstream stream(file);
    stream << hash;
}

static string getFileName(const string& path)
{
    size_t s = path.rfind('/') + 1;
    if (s == string::npos)
        s = 0;
    size_t d = path.rfind('.');
    if (d == string::npos)
        return path.substr(s);
    else
        return path.substr(s, d - s);
}

static void loadBitmaps(const string& root)
{
    static string dot1 = ".";
    static string dot2 = "..";
    
    tinydir_dir dir;
    tinydir_open(&dir, root.data());
    
    while (dir.has_next)
    {
        tinydir_file file;
        tinydir_readfile(&dir, &file);
        
        if (file.is_dir)
        {
            if (dot1 != file.name && dot2 != file.name)
                loadBitmaps(file.path);
        }
        else if (strcmp(file.extension, "png") == 0)
        {
            cout << file.path << endl;
            bitmaps.push_back(new Bitmap(file.path, getFileName(file.path)));
        }
        
        tinydir_next(&dir);
    }
    
    tinydir_close(&dir);
}

int main(int argc, const char* argv[])
{
    //Print out passed arguments
    for (int i = 0; i < argc; ++i)
        cout << argv[i] << ' ';
    cout << endl;
    
    if (argc != 3)
    {
        cerr << "expected 3 parameters \"crunch [INPUT DIRECTORY] [OUTPUT PREFIX]\"" << endl;
        return EXIT_FAILURE;
    }
    
    //Get the input/output directories
    string inputDir = argv[1];
    string outputDir = argv[2];
    string name = getFileName(inputDir);
    inputDir += '/';
    outputDir += '/';
    
    //Hash the input directory
    size_t hash = 0;
    hashFiles(hash, inputDir);
    
    //Load the old hash
    string hashFile = outputDir + name + ".hash";
    size_t oldHash;
    if (loadHash(oldHash, hashFile))
    {
        if (hash == oldHash)
        {
            cout << "images have not changed in: " << inputDir << endl;
            return EXIT_SUCCESS;
        }
    }
    
    //Load the bitmaps and sort them by area
    loadBitmaps(inputDir);
    sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap* a, const Bitmap* b) {
        int areaA = a->width * a->height;
        int areaB = b->width * b->height;
        return areaA > areaB ? -1 : (areaA < areaB ? 1 : 0);
    });
    
    //Pack the bitmaps
    while (!bitmaps.empty())
    {
        auto packer = new Packer(PACK_SIZE, PACK_SIZE);
        packer->Pack(bitmaps);
        packers.push_back(packer);
    }
    
    cout << "saving..." << endl;
    
    ofstream xml(outputDir + name + ".xml");
    xml << "<atlas>" << endl;
    
    //Save the atlases
    for (size_t i = 0, j = packers.size(); i < j; ++i)
        packers[i]->Save(outputDir, name + to_string(i), xml);
    
    xml << "</atlas>";
    
    //Save the new hash
    //saveHash(hash, hashFile);
    
    return EXIT_SUCCESS;
}
