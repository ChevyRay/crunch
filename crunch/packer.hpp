#ifndef packer_hpp
#define packer_hpp

#include <vector>
#include <fstream>
#include <unordered_map>
#include "bitmap.hpp"

using namespace std;

struct Point
{
    int x;
    int y;
    int dupID;
};

struct Packer
{
    int width;
    int height;
    
    vector<Bitmap*> bitmaps;
    vector<Point> points;
    unordered_map<size_t, int> dupLookup;
    
    Packer(int width, int height);
    void Pack(vector<Bitmap*>& bitmaps, bool verbose, int reduce);
    void SavePng(const string& file);
    void SaveXml(const string& name, ofstream& xml);
    void SaveBin(const string& name, ofstream& bin);
};

#endif
