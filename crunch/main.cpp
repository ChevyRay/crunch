#include <iostream>
#include <fstream>
#include <streambuf>
#include <string>
#include <sstream>
#include <vector>
#include "tinydir.h"
#include "bitmap.hpp"
#include "packer.hpp"
#include "binary.hpp"

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

static void loadBitmaps(const string& root, bool premultiply, bool trim)
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
                loadBitmaps(file.path, premultiply, trim);
        }
        else if (strcmp(file.extension, "png") == 0)
        {
            bitmaps.push_back(new Bitmap(file.path, getFileName(file.path), premultiply, trim));
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
    bool binary = false;
    bool binaryXml = false;
    bool premultiply = false;
    bool trim = false;
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
    }
    
    //Hash the input directory
    size_t hash = 0;
    hashFiles(hash, inputDir);
    
    //Load the old hash
    size_t oldHash;
    if (loadHash(oldHash, outputDir + name + ".hash"))
    {
        if (hash == oldHash)
        {
            //cout << "images have not changed in: " << inputDir << endl;
            cout << "atlas is unchanged: " << name << endl;
            return EXIT_SUCCESS;
        }
    }
    
    //Load the bitmaps and sort them by area
    loadBitmaps(inputDir, premultiply, trim);
    sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap* a, const Bitmap* b) {
        return (a->width * a->height) < (b->width * b->height);
    });
    
    //Pack the bitmaps
    while (!bitmaps.empty())
    {
        auto packer = new Packer(PACK_SIZE, PACK_SIZE);
        packer->Pack(bitmaps);
        packers.push_back(packer);
    }
    
    //Remove old files
    RemoveFile(outputDir + name + ".hash");
    RemoveFile(outputDir + name + ".bin");
    RemoveFile(outputDir + name + ".xml");
    for (size_t i = 0; i < 16; ++i)
        RemoveFile(outputDir + name + to_string(i) + ".png");
    
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
        cout << "building atlas: " << name << to_string(i) << endl;
        packers[i]->SavePng(outputDir + name + to_string(i) + ".png");
    }
    
    //Save the atlas binary
    if (binary || binaryXml)
    {
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
        ofstream xml(outputDir + name + ".xml");
        xml << "<atlas>" << endl;
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveXml(name + to_string(i), xml);
        xml << "</atlas>";
    }
    
    //Save the new hash
    saveHash(hash, outputDir + name + ".hash");
    
    return EXIT_SUCCESS;
}
