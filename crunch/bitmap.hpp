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
    void CopyPixelsRot(const Bitmap* src, int tx, int ty);
    bool Equals(const Bitmap* other) const;
};

#endif
