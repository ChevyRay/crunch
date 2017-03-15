#include "packer.hpp"
#include "MaxRectsBinPack.h"
#include "GuillotineBinPack.h"
#include "binary.hpp"
#include <iostream>
#include <algorithm>

using namespace std;
using namespace rbp;

Packer::Packer(int width, int height)
: width(width), height(height)
{
    
}

void Packer::Pack(vector<Bitmap*>& bitmaps, bool verbose)
{
    MaxRectsBinPack packer(width, height);
    
    int ww = 0;
    int hh = 0;
    while (!bitmaps.empty())
    {
        auto bitmap = bitmaps.back();
        
        if (verbose)
            cout << '\t' << bitmaps.size() << ": " << bitmap->name << endl;
        
        //Check to see if this is a duplicate of an already packed bitmap
        auto di = dupLookup.find(bitmap->hash);
        if (di != dupLookup.end())
        {
            Point p = points[di->second];
            points.push_back({ p.x, p.y, di->second });
        }
        else
        {
            Rect rect = packer.Insert(bitmap->width + 1, bitmap->height + 1, MaxRectsBinPack::RectBestShortSideFit);
            
            if (rect.width == 0 || rect.height == 0)
                return;
            
            dupLookup[bitmap->hash] = static_cast<int>(points.size());
            points.push_back({ rect.x, rect.y, -1 });
            
            ww = max(rect.x + rect.width, ww);
            hh = max(rect.y + rect.height, hh);
        }
        
        this->bitmaps.push_back(bitmap);
        bitmaps.pop_back();
    }
    
    while (width / 2 >= ww)
        width /= 2;
    while( height / 2 >= hh)
        height /= 2;
}

void Packer::SavePng(const string& file)
{
    Bitmap bitmap(width, height);
    for (size_t i = 0, j = bitmaps.size(); i < j; ++i)
        if (points[i].dupID < 0)
            bitmap.CopyPixels(bitmaps[i], points[i].x, points[i].y);
    bitmap.SaveAs(file);
}

void Packer::SaveXml(const string& name, ofstream& xml)
{
    xml << "\t<tex n=\"" << name << "\">" << endl;
    for (size_t i = 0, j = bitmaps.size(); i < j; ++i)
    {
        xml << "\t\t<img n=\"" << bitmaps[i]->name << "\" ";
        xml << "x=\"" << points[i].x << "\" ";
        xml << "y=\"" << points[i].y << "\" ";
        xml << "w=\"" << bitmaps[i]->width << "\" ";
        xml << "h=\"" << bitmaps[i]->height << "\" ";
        xml << "fx=\"" << bitmaps[i]->frameX << "\" ";
        xml << "fy=\"" << bitmaps[i]->frameY << "\" ";
        xml << "fw=\"" << bitmaps[i]->frameW << "\" ";
        xml << "fh=\"" << bitmaps[i]->frameH << "\"/>" << endl;
    }
    xml << "\t</tex>" << endl;
}

void Packer::SaveBin(const string& name, ofstream& bin)
{
    WriteString(bin, name);
    WriteShort(bin, (int16_t)bitmaps.size());
    for (size_t i = 0, j = bitmaps.size(); i < j; ++i)
    {
        WriteString(bin, bitmaps[i]->name);
        WriteShort(bin, (int16_t)points[i].x);
        WriteShort(bin, (int16_t)points[i].x);
        WriteShort(bin, (int16_t)bitmaps[i]->width);
        WriteShort(bin, (int16_t)bitmaps[i]->height);
        WriteShort(bin, (int16_t)bitmaps[i]->frameX);
        WriteShort(bin, (int16_t)bitmaps[i]->frameY);
        WriteShort(bin, (int16_t)bitmaps[i]->frameW);
        WriteShort(bin, (int16_t)bitmaps[i]->frameH);
    }
}
