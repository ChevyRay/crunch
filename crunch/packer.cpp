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

#include "packer.hpp"

#include <algorithm>
#include <iostream>

#include "third_party/MaxRectsBinPack.h"
#include "binary.hpp"
#include "options.hpp"

using namespace std;
using namespace rbp;

Packer::Packer(int width, int height, int pad, int stretch)
    : width(width), height(height), pad(pad), stretch(stretch)
{
}

void Packer::Pack(vector<Bitmap *> &bitmaps, bool unique, bool rotate, MaxRectsBinPack::FreeRectChoiceHeuristic choiceHeuristic)
{
    MaxRectsBinPack packer(width + pad, height + pad, rotate);

    int ww = 0, hh = 0;
    int expandAmount = pad + stretch * 2;
    while (!bitmaps.empty())
    {
        auto bitmap = bitmaps.back();

        if (options.verbose)
            cout << '\t' << bitmaps.size() << ": " << bitmap->name << endl;

        // Check to see if this is a duplicate of an already packed bitmap
        if (unique)
        {
            auto di = dupLookup.find(bitmap->hashValue);
            if (di != dupLookup.end() && bitmap->Equals(this->bitmaps[di->second]))
            {
                Point p = points[di->second];
                p.dupID = di->second;
                points.push_back(p);
                this->bitmaps.push_back(bitmap);
                bitmaps.pop_back();
                continue;
            }
        }

        // If it's not a duplicate, pack it into the atlas

        Rect rect = packer.Insert(bitmap->width + expandAmount, bitmap->height + expandAmount, choiceHeuristic);

        if (rect.width == 0 || rect.height == 0)
            break;

        if (unique)
            dupLookup[bitmap->hashValue] = static_cast<int>(points.size());

        // Check if we rotated it
        Point p;
        p.x = rect.x + stretch;
        p.y = rect.y + stretch;
        p.dupID = -1;
        p.rot = rotate && bitmap->width != (rect.width - expandAmount);

        points.push_back(p);
        this->bitmaps.push_back(bitmap);
        bitmaps.pop_back();

        ww = max(rect.x + rect.width - pad, ww);
        hh = max(rect.y + rect.height - pad, hh);
    }

    while (width / 2 >= ww)
        width /= 2;
    while (height / 2 >= hh)
        height /= 2;
}

void Packer::SavePng(const string &file)
{
    Bitmap bitmap(width, height);
    for (int i = 0, j = bitmaps.size(); i < j; ++i)
    {
        if (points[i].dupID >= 0)
            continue;

        int x = points[i].x, y = points[i].y;
        auto bmap = bitmaps[i];

        if (points[i].rot)
            bitmap.CopyPixelsRot(bmap, x, y);
        else
            bitmap.CopyPixels(bmap, x, y);

        if (stretch != 0)
            bitmap.StretchPixels(x, y, bmap->width, bmap->height, stretch);
    }
    bitmap.SaveAs(file);
}

void Packer::SaveXml(const string &name, ofstream &xml, bool trim, bool rotate)
{
    xml << "\t<tex n=\"" << name << "\">" << endl;
    for (int i = 0, j = bitmaps.size(); i < j; ++i)
    {
        xml << "\t\t<img n=\"" << bitmaps[i]->name << "\" ";
        xml << "x=\"" << points[i].x << "\" ";
        xml << "y=\"" << points[i].y << "\" ";
        xml << "w=\"" << bitmaps[i]->width << "\" ";
        xml << "h=\"" << bitmaps[i]->height << "\" ";
        if (trim)
        {
            xml << "fx=\"" << bitmaps[i]->frameX << "\" ";
            xml << "fy=\"" << bitmaps[i]->frameY << "\" ";
            xml << "fw=\"" << bitmaps[i]->frameW << "\" ";
            xml << "fh=\"" << bitmaps[i]->frameH << "\" ";
        }
        if (rotate)
            xml << "r=\"" << (points[i].rot ? 1 : 0) << "\" ";
        xml << "/>" << endl;
    }
    xml << "\t</tex>" << endl;
}

void Packer::SaveBin(const string &name, ofstream &bin, bool trim, bool rotate)
{
    WriteString(bin, name);
    WriteShort(bin, (int16_t)bitmaps.size());
    for (int i = 0, j = bitmaps.size(); i < j; ++i)
    {
        WriteString(bin, bitmaps[i]->name);
        WriteShort(bin, (int16_t)points[i].x);
        WriteShort(bin, (int16_t)points[i].y);
        WriteShort(bin, (int16_t)bitmaps[i]->width);
        WriteShort(bin, (int16_t)bitmaps[i]->height);
        if (trim)
        {
            WriteShort(bin, (int16_t)bitmaps[i]->frameX);
            WriteShort(bin, (int16_t)bitmaps[i]->frameY);
            WriteShort(bin, (int16_t)bitmaps[i]->frameW);
            WriteShort(bin, (int16_t)bitmaps[i]->frameH);
        }
        if (rotate)
            WriteByte(bin, points[i].rot ? 1 : 0);
    }
}

void Packer::SaveJson(const string &name, ofstream &json, bool trim, bool rotate)
{
    json << "\t\t\"" << name << "\": {" << endl;
    for (int i = 0, j = bitmaps.size(); i < j; ++i)
    {
        json << "\t\t\t\"" << bitmaps[i]->name << "\": { ";
        json << "\"x\": " << points[i].x << ", ";
        json << "\"y\": " << points[i].y << ", ";
        json << "\"w\": " << bitmaps[i]->width << ", ";
        json << "\"h\": " << bitmaps[i]->height;
        if (trim)
        {
            json << ", \"fx\": " << bitmaps[i]->frameX << ", ";
            json << "\"fy\": " << bitmaps[i]->frameY << ", ";
            json << "\"fw\": " << bitmaps[i]->frameW << ", ";
            json << "\"fh\": " << bitmaps[i]->frameH;
        }
        if (rotate)
            json << ", \"r\": " << (points[i].rot ? "true" : "false");
        json << " }";
        if (i != bitmaps.size() - 1)
            json << ",";
        json << endl;
    }
    json << "\t\t}";
}
