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
 
 crunch - command line texture packer
 ====================================
 
 usage:
    crunch [INPUT_DIR] [OUTPUT_DIR] [OPTIONS...]
 
 example:
    crunch assets/characters bin/atlases -p -t -v -u -r
 
 options:
    -b  --binary            saves the atlas data as .bin file (instead of xml)
    -bx --binaryxml         saves the atlas data as both .bin file and .xml file
    -p  --premultiply       premultiplies the pixels of the bitmaps by their alpha channel
    -t  --trim              trims excess transparency off the bitmaps
    -v  --verbose           print to the debug console as the packer works
    -f  --force             ignore the hash, forcing the packer to repack
    -u  --unique            removes duplicate bitmaps from the atlas by hash comparison
    -r  --rotate            enabled rotating bitmaps 90 degrees clockwise when packing
 
 binary format:
    [int16] num_textures (below block is repeated this many times)
        [string] name
        [int16] num_images (below block is repeated this many times)
            [string] img_name
            [int16] img_x
            [int16] img_y
            [int16] img_width
            [int16] img_height
            [int16] img_frame_x         (if --trim enabled)
            [int16] img_frame_y         (if --trim enabled)
            [int16] img_frame_width     (if --trim enabled)
            [int16] img_frame_height    (if --trim enabled)
            [byte] img_rotated          (if --rotate enabled)
 */

#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <vector>
#include <algorithm>
#include "tinydir.h"
#include "bitmap.hpp"
#include "packer.hpp"
#include "binary.hpp"
#include "hash.hpp"
#include "str.hpp"

#define PACK_SIZE 4096

using namespace std;

static bool optBinary;
static bool optBinaryXml;
static bool optPremultiply;
static bool optTrim;
static bool optVerbose;
static bool optForce;
static bool optUnique;
static bool optRotate;
static vector<Bitmap*> bitmaps;
static vector<Packer*> packers;

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

static void loadBitmaps(const string& root, const string& prefix)
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
                loadBitmaps(FileToStr(file.path), prefix + FileToStr(file.name) + "/");
        }
        else if (FileToStr(file.extension) == "png")
        {
            if (optVerbose)
                cout << '\t' << FileToStr(file.path) << endl;
            
            bitmaps.push_back(new Bitmap(FileToStr(file.path), prefix + getFileName(FileToStr(file.path)), optPremultiply, optTrim));
        }
        
        tinydir_next(&dir);
    }
    
    tinydir_close(&dir);
}

static void RemoveFile(string file)
{
    remove(file.data());
}

int main(int argc, const char* argv[])
{
    //Print out passed arguments
    for (int i = 0; i < argc; ++i)
        cout << argv[i] << ' ';
    cout << endl;
    
    if (argc < 3)
    {
        cerr << "invalid input, expected: \"crunch [INPUT DIRECTORY] [OUTPUT PREFIX] [OPTIONS...]\"" << endl;
        return EXIT_FAILURE;
    }
    
    //Get the input/output directories
    string inputDir = argv[1];
    string outputDir = argv[2];
    string name = getFileName(inputDir);
    inputDir += '/';
    outputDir += '/';
    
    //Get the options
    optBinary = false;
    optBinaryXml = false;
    optPremultiply = false;
    optTrim = false;
    optVerbose = false;
    optForce = false;
    optUnique = false;
    for (int i = 3; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "-b" || arg == "--binary")
            optBinary = true;
        if (arg == "-bx" || arg == "--binaryxml")
            optBinaryXml = true;
        if (arg == "-p" || arg == "--premultiply")
            optPremultiply = true;
        if (arg == "-t" || arg == "--trim")
            optTrim = true;
        if (arg == "-v" || arg == "--verbose")
            optVerbose = true;
        if (arg == "-f" || arg == "--force")
            optForce = true;
        if (arg == "-u" || arg == "--unique")
            optUnique = true;
        if (arg == "-r" || arg == "--rotate")
            optRotate = true;
    }
    
    //Hash the input directory
    size_t newHash = 0;
    hashFiles(newHash, inputDir);
    
    //Load the old hash
    size_t oldHash;
    if (loadHash(oldHash, outputDir + name + ".hash"))
    {
        if (!optForce && newHash == oldHash)
        {
            cout << "atlas is unchanged: " << name << endl;
            return EXIT_SUCCESS;
        }
    }
    
    //Make sure output directory exists.
    struct stat fileStat;
    if(stat(outputDir.c_str(), &fileState) == -1) {
        mkdir(outputDir.c_str(), 0775); //May only work on OSX.
    }
    
    //Remove old files
    RemoveFile(outputDir + name + ".hash");
    RemoveFile(outputDir + name + ".bin");
    RemoveFile(outputDir + name + ".xml");
    for (size_t i = 0; i < 16; ++i)
        RemoveFile(outputDir + name + to_string(i) + ".png");
    
    //Load the bitmaps and sort them by area
    if (optVerbose)
        cout << "loading images..." << endl;
    loadBitmaps(inputDir, "");
    sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap* a, const Bitmap* b) {
        return (a->width * a->height) < (b->width * b->height);
    });
    
    //Pack the bitmaps
    while (!bitmaps.empty())
    {
        if (optVerbose)
            cout << "packing " << bitmaps.size() << " images..." << endl;
        auto packer = new Packer(PACK_SIZE, PACK_SIZE);
        packer->Pack(bitmaps, optVerbose, optUnique, optRotate);
        packers.push_back(packer);
        if (optVerbose)
            cout << "\tfinished packing: " << name << to_string(packers.size() - 1) << " (" << packer->width << " x " << packer->height << ')' << endl;
    }
    
    //Save the atlas image
    for (size_t i = 0; i < packers.size(); ++i)
    {
        if (optVerbose)
            cout << "writing png: " << outputDir << name << to_string(i) << ".png" << endl;
        packers[i]->SavePng(outputDir + name + to_string(i) + ".png");
    }
    
    //Save the atlas binary
    if (optBinary || optBinaryXml)
    {
        if (optVerbose)
            cout << "writing bin: " << outputDir << name << ".bin" << endl;
        
        ofstream bin(outputDir + name + ".bin", ios::binary);
        WriteShort(bin, (int16_t)packers.size());
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveBin(name + to_string(i), bin, optTrim, optRotate);
        bin.close();
    }
    
    //Save the atlas xml
    if (!optBinary || optBinaryXml)
    {
        if (optVerbose)
            cout << "writing xml: " << outputDir << name << ".xml" << endl;
        
        ofstream xml(outputDir + name + ".xml");
        xml << "<atlas>" << endl;
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveXml(name + to_string(i), xml, optTrim, optRotate);
        xml << "</atlas>";
    }
    
    //Save the new hash
    saveHash(newHash, outputDir + name + ".hash");
    
    return EXIT_SUCCESS;
}
