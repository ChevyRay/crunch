#ifndef packer_hpp
#define packer_hpp

#include <vector>
#include <fstream>
#include "bitmap.hpp"

using namespace std;

struct Point
{
    int x;
    int y;
};

struct Packer
{
    int width;
    int height;
    vector<Bitmap*> bitmaps;
    vector<Point> points;
    
    Packer(int width, int height);
    void Pack(vector<Bitmap*>& bitmaps);
    void SavePng(const string& file);
    void SaveXml(const string& name, ofstream& xml);
    void SaveBin(const string& name, ofstream& bin);
};

#endif
