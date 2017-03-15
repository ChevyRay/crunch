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

static bool binary;
static bool binaryXml;
static bool premultiply;
static bool trim;
static bool verbose;
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
            if (verbose)
                cout << '\t' << FileToStr(file.path) << endl;
            
            bitmaps.push_back(new Bitmap(FileToStr(file.path), prefix + getFileName(FileToStr(file.path)), premultiply, trim));
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
    //string inputDir = "/Users/ChevyRay/Desktop/crunch_test/assets/chars";
    //string outputDir = "/Users/ChevyRay/Desktop/crunch_test/bin";
    string name = getFileName(inputDir);
    inputDir += '/';
    outputDir += '/';
    
    //Get the options
    binary = false;
    binaryXml = false;
    premultiply = false;
    trim = false;
    verbose = false;
    for (int i = 3; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "-b" || arg == "--binary")
            binary = true;
        if (arg == "-bx" || arg == "--binaryxml")
            binaryXml = true;
        if (arg == "-p" || arg == "--premultiply")
            premultiply = true;
        if (arg == "-t" || arg == "--trim")
            trim = true;
        if (arg == "-v" || arg == "--verbose")
            verbose = true;
    }
    
    //Hash the input directory
    size_t newHash = 0;
    hashFiles(newHash, inputDir);
    
    //Load the old hash
    size_t oldHash;
    if (loadHash(oldHash, outputDir + name + ".hash"))
    {
        if (newHash == oldHash)
        {
            cout << "atlas is unchanged: " << name << endl;
            return EXIT_SUCCESS;
        }
    }
    
    //Remove old files
    RemoveFile(outputDir + name + ".hash");
    RemoveFile(outputDir + name + ".bin");
    RemoveFile(outputDir + name + ".xml");
    for (size_t i = 0; i < 16; ++i)
        RemoveFile(outputDir + name + to_string(i) + ".png");
    
    //Load the bitmaps and sort them by area
    if (verbose)
        cout << "loading images..." << endl;
    loadBitmaps(inputDir, "");
    sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap* a, const Bitmap* b) {
        return (a->width * a->height) < (b->width * b->height);
    });
    
    //Pack the bitmaps
    while (!bitmaps.empty())
    {
        if (verbose)
            cout << "packing " << bitmaps.size() << " images..." << endl;
        auto packer = new Packer(PACK_SIZE, PACK_SIZE);
        packer->Pack(bitmaps, verbose);
        packers.push_back(packer);
        if (verbose)
            cout << "\tfinished packing: " << name << to_string(packers.size() - 1) << " (" << packer->width << " x " << packer->height << ')' << endl;
    }
    
    //FORMAT:
    //num_textures (int16)
    //TEXTURE:
    //  name (string)
    //  num_images (int16)
    //  IMAGES:
    //      name (string)
    //      x, y, fx, fy, fw, fh (int16)
    
    //Save the atlas image
    for (size_t i = 0; i < packers.size(); ++i)
    {
        if (verbose)
            cout << "writing png: " << outputDir << name << to_string(i) << ".png" << endl;
        packers[i]->SavePng(outputDir + name + to_string(i) + ".png");
    }
    
    //Save the atlas binary
    if (binary || binaryXml)
    {
        if (verbose)
            cout << "writing bin: " << outputDir << name << ".bin" << endl;
        
        ofstream bin(outputDir + name + ".bin", ios::binary);
        WriteShort(bin, (int16_t)packers.size());
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveBin(name + to_string(i), bin);
        bin.close();
        
        //Test binary output
        /*ifstream in(outputDir + name + ".bin");
        int16_t num_textures = ReadShort(in);
        cout << "num_textures: " << num_textures << endl;
        for (int16_t i = 0; i < num_textures; ++i)
        {
            string tex_name = ReadString(in);
            cout << "\ttex_name: " << tex_name << endl;
            int16_t num_images = ReadShort(in);
            cout << "\tnum_images: " << num_images << endl;
            for (int16_t j = 0; j < num_images; ++j)
            {
                cout << "\t\t" << ReadString(in)
                << " x: " << ReadShort(in)
                << " y: " << ReadShort(in)
                << " w: " << ReadShort(in)
                << " h: " << ReadShort(in)
                << " fx: " << ReadShort(in)
                << " fy: " << ReadShort(in)
                << " fw: " << ReadShort(in)
                << " fh: " << ReadShort(in) << endl;
            }
        }*/
    }
    
    //Save the atlas xml
    if (!binary || binaryXml)
    {
        if (verbose)
            cout << "writing xml: " << outputDir << name << ".xml" << endl;
        
        ofstream xml(outputDir + name + ".xml");
        xml << "<atlas>" << endl;
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveXml(name + to_string(i), xml);
        xml << "</atlas>";
    }
    
    //Save the new hash
    saveHash(newHash, outputDir + name + ".hash");
    
    return EXIT_SUCCESS;
}
