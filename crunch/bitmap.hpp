#ifndef bitmap_hpp
#define bitmap_hpp

#include <string>
#include <cstdint>
#include <vector>

using namespace std;

struct Bitmap
{
    string name;
    int width;
    int height;
    int frameX;
    int frameY;
    int frameW;
    int frameH;
    uint32_t* data;
    size_t hashValue;
    Bitmap(const string& file, const string& name, bool premultiply, bool trim);
    Bitmap(int width, int height);
    ~Bitmap();
    void SaveAs(const string& file);
    void CopyPixels(const Bitmap* src, int tx, int ty);
};

#endif
