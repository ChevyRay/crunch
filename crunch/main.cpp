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
    crunch [OUTPUT] [INPUT1,INPUT2,INPUT3...] [OPTIONS...]

 example:
    crunch bin/atlases/atlas assets/characters,assets/tiles -p -t -v -u -r

 options:
    -d    --default           use default settings (-x -p -t -u)
    -x    --xml               saves the atlas data as a .xml file
    -b    --binary            saves the atlas data as a .bin file
    -j    --json              saves the atlas data as a .json file
    -p    --premultiply       premultiplies the pixels of the bitmaps by their alpha channel
    -t    --trim              trims excess transparency off the bitmaps
    -v    --verbose           print to the debug console as the packer works
    -f    --force             ignore the hash, forcing the packer to repack
    -u    --unique            remove duplicate bitmaps from the atlas
    -r    --rotate            enabled rotating bitmaps 90 degrees clockwise when packing
    -s#   --size#             max atlas size (# can be 4096, 2048, 1024, 512, 256, 128, or 64)
    -p#   --pad#              padding between images (# can be from 0 to 16)
    -bs%  --binstr%           string type in binary format (% can be: n - null-termainated, p - prefixed (int16), 7 - 7-bit prefixed)
    -tm   --time              use file's last write time instead of its content for hashing
    -sp   --split             split output textures by subdirectories
    -nz   --nozero            if there's ony one packed texture, then zero at the end of its name will be omitted (ex. images0.png -> images.png
    
    binary format:
    crch (0x68637263 in hex or 1751347811 in decimal)
    [int16] version (current version is 0)
    [byte] --trim enabled
    [byte] --rotate enabled
    [byte] string type (0 - null-termainated, 1 - prefixed (int16), 2 - 7-bit prefixed)
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
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include "tinydir.h"
#include "bitmap.hpp"
#include "packer.hpp"
#include "binary.hpp"
#include "hash.hpp"
#include "str.hpp"
#include "time.hpp"
#define EXIT_SKIPPED 2

using namespace std;

const char *version = "v0.1";
const int binVersion = 0;

static int optSize;
static int optPadding;
static int optBinstr;
static bool optXml;
static bool optBinary;
static bool optJson;
static bool optPremultiply;
static bool optTrim;
static bool optVerbose;
static bool optForce;
static bool optUnique;
static bool optRotate;
static bool optTime;
static bool optSplit;
static bool optNoZero;
static vector<Bitmap *> bitmaps;
static vector<Packer *> packers;

static const char *helpMessage =
    "usage:\n"
    "   crunch [OUTPUT] [INPUT1,INPUT2,INPUT3...] [OPTIONS...]\n"
    "\n"
    "example:\n"
    "   crunch bin/atlases/atlas assets/characters,assets/tiles -p -t -v -u -r\n"
    "\n"
    "options:\n"
    "   -d    --default           use default settings (-x -p -t -u)\n"
    "   -x    --xml               saves the atlas data as a .xml file\n"
    "   -b    --binary            saves the atlas data as a .bin file\n"
    "   -j    --json              saves the atlas data as a .json file\n"
    "   -p    --premultiply       premultiplies the pixels of the bitmaps by their alpha channel\n"
    "   -t    --trim              trims excess transparency off the bitmaps\n"
    "   -v    --verbose           print to the debug console as the packer works\n"
    "   -f    --force             ignore the hash, forcing the packer to repack\n"
    "   -u    --unique            remove duplicate bitmaps from the atlas\n"
    "   -r    --rotate            enabled rotating bitmaps 90 degrees clockwise when packing\n"
    "   -s#   --size#             max atlas size (# can be 4096, 2048, 1024, 512, 256, 128, or 64)\n"
    "   -p#   --pad#              padding between images (# can be from 0 to 16)\n"
    "   -bs%  --binstr%           string type in binary format (% can be: n - null-termainated, p - prefixed (int16), 7 - 7-bit prefixed)\n"
    "   -tm   --time              use file's last write time instead of its content for hashing\n"
    "   -sp   --split             split output textures by subdirectories\n"
    "   -nz   --nozero            if there's ony one packed texture, then zero at the end of its name will be omitted (ex. images0.png -> images.png)\n"
    "\n"
    "binary format:\n"
    "crch (0x68637263 in hex or 1751347811 in decimal)\n"
    "[int16] version (current version is 0)"
    "[byte] --trim enabled\n"
    "[byte] --rotate enabled\n"
    "[byte] string type (0 - null-termainated, 1 - prefixed (int16), 2 - 7-bit prefixed)\n"
    "[int16] num_textures (below block is repeated this many times)\n"
    "  [string] name\n"
    "    [int16] num_images (below block is repeated this many times)\n"
    "      [string] img_name\n"
    "      [int16] img_x\n"
    "      [int16] img_y\n"
    "      [int16] img_width\n"
    "      [int16] img_height\n"
    "      [int16] img_frame_x         (if --trim enabled)\n"
    "      [int16] img_frame_y         (if --trim enabled)\n"
    "      [int16] img_frame_width     (if --trim enabled)\n"
    "      [int16] img_frame_height    (if --trim enabled)\n"
    "      [byte] img_rotated          (if --rotate enabled)";

static void SplitFileName(const string &path, string *dir, string *name, string *ext)
{
    size_t si = path.rfind('/') + 1;
    if (si == string::npos)
        si = 0;
    size_t di = path.rfind('.');
    if (dir != nullptr)
    {
        if (si > 0)
            *dir = path.substr(0, si);
        else
            *dir = "";
    }
    if (name != nullptr)
    {
        if (di != string::npos)
            *name = path.substr(si, di - si);
        else
            *name = path.substr(si);
    }
    if (ext != nullptr)
    {
        if (di != string::npos)
            *ext = path.substr(di);
        else
            *ext = "";
    }
}

static string GetFileName(const string &path)
{
    string name;
    SplitFileName(path, nullptr, &name, nullptr);
    return name;
}

static void LoadBitmap(const string &prefix, const string &path)
{
    if (optVerbose)
        cout << '\t' << path << endl;

    bitmaps.push_back(new Bitmap(path, prefix + GetFileName(path), optPremultiply, optTrim, optVerbose));
}

static void LoadBitmaps(const string &root, const string &prefix)
{
    static string dot1 = ".";
    static string dot2 = "..";

    tinydir_dir dir;
    tinydir_open_sorted(&dir, StrToPath(root).data());

    for (int i = 0; i < static_cast<int>(dir.n_files); ++i)
    {
        tinydir_file file;
        tinydir_readfile_n(&dir, &file, i);

        if (file.is_dir)
        {
            if (dot1 != PathToStr(file.name) && dot2 != PathToStr(file.name))
                LoadBitmaps(PathToStr(file.path), prefix + PathToStr(file.name) + "/");
        }
        else if (PathToStr(file.extension) == "png")
            LoadBitmap(prefix, PathToStr(file.path));
    }

    tinydir_close(&dir);
}

static void RemoveFile(string file)
{
    remove(file.data());
}

static int GetPackSize(const string &str)
{
    if (str == "4096")
        return 4096;
    if (str == "2048")
        return 2048;
    if (str == "1024")
        return 1024;
    if (str == "512")
        return 512;
    if (str == "256")
        return 256;
    if (str == "128")
        return 128;
    if (str == "64")
        return 64;
    cerr << "invalid size: " << str << endl;
    exit(EXIT_FAILURE);
    return 0;
}

static int GetBinStrType(const string &str)
{
    if (str == "n" || str == "N")
        return 0;
    if (str == "p" || str == "P")
        return 1;
    if (str == "7")
        return 2;
    cerr << "invalid binary string type: " << str << endl;
    exit(EXIT_FAILURE);
    return 0;
}

static int GetPadding(const string &str)
{
    for (int i = 0; i <= 16; ++i)
        if (str == to_string(i))
            return i;
    cerr << "invalid padding value: " << str << endl;
    exit(EXIT_FAILURE);
    return 1;
}

static void GetSubdirs(const string &root, vector<string> &subdirs)
{
    static string dot1 = ".";
    static string dot2 = "..";

    tinydir_dir dir;
    tinydir_open_sorted(&dir, StrToPath(root).data());

    for (int i = 0; i < static_cast<int>(dir.n_files); ++i)
    {
        tinydir_file file;
        tinydir_readfile_n(&dir, &file, i);

        if (file.is_dir)
        {
            if (dot1 != PathToStr(file.name) && dot2 != PathToStr(file.name))
                subdirs.push_back(PathToStr(file.path));
        }
    }

    tinydir_close(&dir);
}

static void FindPackers(const string &root, const string &name, const string &ext, vector<string> &packers)
{
    static string dot1 = ".";
    static string dot2 = "..";

    tinydir_dir dir;
    tinydir_open_sorted(&dir, StrToPath(root).data());

    for (int i = 0; i < static_cast<int>(dir.n_files); ++i)
    {
        tinydir_file file;
        tinydir_readfile_n(&dir, &file, i);

        if (!file.is_dir && PathToStr(file.name).starts_with(name) && PathToStr(file.extension) == ext)
            packers.push_back(PathToStr(file.path));
    }

    tinydir_close(&dir);
}

static int Pack(size_t newHash, string &outputDir, string &name, vector<string> &inputs, string prefix = "")
{
    if (optSplit) StartTimer(prefix);
    StartTimer("hashing input");
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        if (inputs[i].rfind('.') == string::npos)
            HashFiles(newHash, inputs[i], optTime);
        else
            HashFile(newHash, inputs[i], optTime);
    }
    StopTimer("hashing input");
    // Load the old hash
    size_t oldHash;
    if (LoadHash(oldHash, outputDir + name + ".hash"))
    {
        if (!optForce && newHash == oldHash)
        {
            if (!optSplit)
            {
                cout << "atlas is unchanged: " << name << endl;
                
                return EXIT_SUCCESS;
            }
            StopTimer(prefix);
            return EXIT_SKIPPED;
        }
    }

    // Remove old files
    RemoveFile(outputDir + name + ".hash");
    RemoveFile(outputDir + name + ".bin");
    RemoveFile(outputDir + name + ".xml");
    RemoveFile(outputDir + name + ".json");
    RemoveFile(outputDir + name + ".png");
    for (size_t i = 0; i < 16; ++i)
        RemoveFile(outputDir + name + to_string(i) + ".png");

    StartTimer("loading bitmaps");

    // Load the bitmaps from all the input files and directories
    if (optVerbose)
        cout << "loading images..." << endl;
    for (size_t i = 0; i < inputs.size(); ++i)
    {
        if (!optSplit && inputs[i].rfind('.') != string::npos)
            LoadBitmap("", inputs[i]);
        else
            LoadBitmaps(inputs[i], prefix);
    }
    StopTimer("loading bitmaps");

    StartTimer("sorting bitmaps");
    // Sort the bitmaps by area
    stable_sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap* a, const Bitmap* b)
        { return (a->width * a->height) < (b->width * b->height); });
    StopTimer("sorting bitmaps");

    StartTimer("packing bitmaps");
    // Pack the bitmaps
    while (!bitmaps.empty())
    {
        if (optVerbose)
            cout << "packing " << bitmaps.size() << " images..." << endl;
        auto packer = new Packer(optSize, optSize, optPadding);
        packer->Pack(bitmaps, optVerbose, optUnique, optRotate);
        packers.push_back(packer);
        if (optVerbose)
            cout << "finished packing: " << name << (optNoZero && bitmaps.empty() ? "" : to_string(packers.size() - 1)) << " (" << packer->width << " x " << packer->height << ')' << endl;

        if (packer->bitmaps.empty())
        {
            cerr << "packing failed, could not fit bitmap: " << (bitmaps.back())->name << endl;
            return EXIT_FAILURE;
        }
    }
    StopTimer("packing bitmaps");

    bool noZero = optNoZero && packers.size() == 1;

    StartTimer("saving atlas png");
    // Save the atlas image
    for (size_t i = 0; i < packers.size(); ++i)
    {
        string pngName = outputDir + name + (noZero ? "" : to_string(i)) + ".png";
        if (optVerbose)
            cout << "writing png: " << pngName << endl;
        packers[i]->SavePng(pngName);
    }
    StopTimer("saving atlas png");

    StartTimer("saving atlas");
    // Save the atlas binary
    if (optBinary)
    {
        SetStringType(optBinstr);
        if (optVerbose)
            cout << "writing bin: " << outputDir << name << ".bin" << endl;

        ofstream bin(outputDir + name + ".bin", ios::binary);
        
        if (!optSplit)
        {
            WriteByte(bin, 'c');
            WriteByte(bin, 'r');
            WriteByte(bin, 'c');
            WriteByte(bin, 'h');
            WriteShort(bin, binVersion);
            WriteByte(bin, optTrim);
            WriteByte(bin, optRotate);
            WriteByte(bin, optBinstr);
        }
        WriteShort(bin, (int16_t)packers.size());
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveBin(name + (noZero ? "" : to_string(i)), bin, optTrim, optRotate);
        bin.close();
    }

    // Save the atlas xml
    if (optXml)
    {
        if (optVerbose)
            cout << "writing xml: " << outputDir << name << ".xml" << endl;

        ofstream xml(outputDir + name + ".xml");
        if (!optSplit)
        {
            xml << "<atlas>" << endl;
            xml << "\t<trim>" << (optTrim ? "true" : "false") << "</trim>" << endl;
            xml << "\t<rotate>" << (optRotate ? "true" : "false") << "</trim>" << endl;
        }
        for (size_t i = 0; i < packers.size(); ++i)
            packers[i]->SaveXml(name + (noZero ? "" : to_string(i)), xml, optTrim, optRotate);
        if (!optSplit) xml << "</atlas>";
        xml.close();
    }

    // Save the atlas json
    if (optJson)
    {
        if (optVerbose)
            cout << "writing json: " << outputDir << name << ".json" << endl;

        ofstream json(outputDir + name + ".json");
        if (!optSplit)
        {
            json << '{' << endl;
            json << "\t\"trim\":" << (optTrim ? "true" : "false") << ',' << endl;
            json << "\t\"rotate\":" << (optRotate ? "true" : "false") << ',' << endl;
            json << "\t\"textures\":[" << endl;
        }
        for (size_t i = 0; i < packers.size(); ++i)
        {
            json << "\t\t{" << endl;
            packers[i]->SaveJson(name + (noZero ? "" : to_string(i)), json, optTrim, optRotate);
            json << "\t\t}";
            if (!optSplit)
            {
                if (i + 1 < packers.size())
                    json << ',';
                json << endl;
            }
        }
        if (!optSplit)
        {
            json << "\t]" << endl;
            json << '}';
        }
        json.close();
    }
    StopTimer("saving atlas");

    // Save the new hash
    SaveHash(newHash, outputDir + name + ".hash");

    if (optSplit) StopTimer(prefix);

    return EXIT_SUCCESS;
}

int main(int argc, const char *argv[])
{
    // Print out passed arguments
    for (int i = 0; i < argc; ++i)
        cout << argv[i] << ' ';
    cout << endl;

    if (argc < 3)
    {
        if (argc == 2)
        {
            string arg = argv[1];
            if (arg == "-h" || arg == "-?" || arg == "--help")
            {
                cout << helpMessage << endl;
                return EXIT_SUCCESS;
            }
            else if (arg == "--version")
            {
                cout << "crunch " << version << endl;
                return EXIT_SUCCESS;
            }
        }
        
        cerr << "invalid input, expected: \"crunch [OUTPUT] [INPUT1,INPUT2,INPUT3...] [OPTIONS...]\"" << endl;
        
        return EXIT_FAILURE;
    }

    StartTimer("total");

    // Get the output directory and name
    string outputDir, name;
    SplitFileName(argv[1], &outputDir, &name, nullptr);

    // Get all the input files and directories
    vector<string> inputs;
    stringstream ss(argv[2]);
    while (ss.good())
    {
        string inputStr;
        getline(ss, inputStr, ',');
        inputs.push_back(inputStr);
    }

    // Get the options
    optSize = 4096;
    optPadding = 1;
    optBinstr = 0;
    optXml = false;
    optBinary = false;
    optJson = false;
    optPremultiply = false;
    optTrim = false;
    optVerbose = false;
    optForce = false;
    optUnique = false;
    optTime = false;
    optSplit = false;
    optNoZero = false;
    for (int i = 3; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "-d" || arg == "--default")
            optXml = optPremultiply = optTrim = optUnique = true;
        else if (arg == "-x" || arg == "--xml")
            optXml = true;
        else if (arg == "-b" || arg == "--binary")
            optBinary = true;
        else if (arg == "-j" || arg == "--json")
            optJson = true;
        else if (arg == "-p" || arg == "--premultiply")
            optPremultiply = true;
        else if (arg == "-t" || arg == "--trim")
            optTrim = true;
        else if (arg == "-v" || arg == "--verbose")
            optVerbose = true;
        else if (arg == "-f" || arg == "--force")
            optForce = true;
        else if (arg == "-u" || arg == "--unique")
            optUnique = true;
        else if (arg == "-r" || arg == "--rotate")
            optRotate = true;
        else if (arg == "-tm" || arg == "--time")
            optTime = true;
        else if (arg == "-sp" || arg == "--split")
            optSplit = true;
        else if (arg == "-nz" || arg == "--nozero")
            optNoZero = true;
        else if (arg.find("-bs") == 0)
            optBinstr = GetBinStrType(arg.substr(3));
        else if (arg.find("--binstr") == 0)
            optBinstr = GetBinStrType(arg.substr(8));
        else if (arg.find("--size") == 0)
            optSize = GetPackSize(arg.substr(6));
        else if (arg.find("-s") == 0)
            optSize = GetPackSize(arg.substr(2));
        else if (arg.find("--pad") == 0)
            optPadding = GetPadding(arg.substr(5));
        else if (arg.find("-p") == 0)
            optPadding = GetPadding(arg.substr(2));
        else
        {
            cerr << "unexpected argument: " << arg << endl;
            return EXIT_FAILURE;
        }
    }

    /*
    -d  --default           use default settings (-x -p -t -u)
    -x  --xml               saves the atlas data as a .xml file
    -b  --binary            saves the atlas data as a .bin file
    -j  --json              saves the atlas data as a .json file
    -p  --premultiply       premultiplies the pixels of the bitmaps by their alpha channel
    -t  --trim              trims excess transparency off the bitmaps
    -v  --verbose           print to the debug console as the packer works
    -f  --force             ignore the hash, forcing the packer to repack
    -u  --unique            remove duplicate bitmaps from the atlas
    -r  --rotate            enabled rotating bitmaps 90 degrees clockwise when packing
    -s# --size#             max atlas size (# can be 4096, 2048, 1024, 512, or 256)
    -p# --pad#              padding between images (# can be from 0 to 16)
    -bs%  --binstr%         string type in binary format (% can be: n - null-termainated, p - prefixed (int16), 7 - 7-bit prefixed)
    -tm   --time            use file's last write time instead of its content for hashing
    -sp   --split           split output textures by subdirectories
    -nz   --nozero          if there's ony one packed texture, then zero at the end of its name will be omitted (ex. images0.png -> images.png)
    */

    if (optVerbose)
    {
        cout << "options..." << endl;
        cout << "\t--xml: " << (optXml ? "true" : "false") << endl;
        cout << "\t--binary: " << (optBinary ? "true" : "false") << endl;
        cout << "\t--json: " << (optJson ? "true" : "false") << endl;
        cout << "\t--premultiply: " << (optPremultiply ? "true" : "false") << endl;
        cout << "\t--trim: " << (optTrim ? "true" : "false") << endl;
        cout << "\t--verbose: " << (optVerbose ? "true" : "false") << endl;
        cout << "\t--force: " << (optForce ? "true" : "false") << endl;
        cout << "\t--unique: " << (optUnique ? "true" : "false") << endl;
        cout << "\t--rotate: " << (optRotate ? "true" : "false") << endl;
        cout << "\t--size: " << optSize << endl;
        cout << "\t--pad: " << optPadding << endl;
        cout << "\t--binstr: " << (optBinstr == 0 ? "n" : (optBinstr == 1 ? "p" : "7")) << endl;
        cout << "\t--time: " << (optTime ? "true" : "false") << endl;
        cout << "\t--split: " << (optSplit ? "true" : "false") << endl;
        cout << "\t--nozero: " << (optNoZero ? "true" : "false") << endl;
    }

    StartTimer("hashing input");
    // Hash the arguments and input directories
    size_t newHash = 0;
    for (int i = 1; i < argc; ++i)
        HashString(newHash, argv[i]);
    StopTimer("hashing input");
    
    if (!optSplit)
    {
        int result = Pack(newHash, outputDir, name, inputs);

        if (result != EXIT_SUCCESS)
            return result;

        StopTimer("total");

        WriteAllTimers();

        return EXIT_SUCCESS;
    }

    string newInput, namePrefix;
    vector<string> subdirs;

    for (string &input : inputs)
    {
        if (!input.ends_with(".png"))
        {
            newInput = input;
            break;
        }
    }

    if (newInput.empty())
    {
        cerr << "could not find directories in input" << endl;
        return EXIT_FAILURE;
    }

    namePrefix = name + "_";

    GetSubdirs(newInput, subdirs);

    bool skipped = true;
    for (string& subdir : subdirs)
    {
        string newName = GetFileName(subdir), prefixedName = namePrefix + newName;
        vector<string> input{ subdir };
        int result = Pack(newHash, outputDir, prefixedName, input, newName + "/");
        if (result == EXIT_SUCCESS)
            skipped = false;
        else if(result != EXIT_SKIPPED)
            return result;

        packers.clear();
        bitmaps.clear();
    }

    if (skipped)
    {
        cout << "atlas is unchanged: " << name << endl;

        StopTimer("total");
        WriteAllTimers();
        return EXIT_SUCCESS;
    }

    RemoveFile(outputDir + name + ".bin");
    RemoveFile(outputDir + name + ".xml");
    RemoveFile(outputDir + name + ".json");

    StartTimer("saving atlas");
    vector<string> cachedPackers;
    if (optBinary)
    {
        SetStringType(optBinstr);
        if (optVerbose)
            cout << "writing bin: " << outputDir << name << ".bin" << endl;

        vector<ifstream*> cacheFiles;

        FindPackers(outputDir, namePrefix, "bin", cachedPackers);

        ofstream bin(outputDir + name + ".bin", ios::binary);
        WriteByte(bin, 'c');
        WriteByte(bin, 'r');
        WriteByte(bin, 'c');
        WriteByte(bin, 'h');
        WriteShort(bin, binVersion);
        WriteByte(bin, optTrim);
        WriteByte(bin, optRotate);
        WriteByte(bin, optBinstr);
        int16_t imageCount = 0;
        for (size_t i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream binCache(cachedPackers[i], ios::binary);
            imageCount += ReadShort(binCache);
            binCache.close();
        }
        WriteShort(bin, imageCount);
        for (size_t i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream binCache(cachedPackers[i], ios::binary);
            ReadShort(binCache);
            bin << binCache.rdbuf();
            binCache.close();
        }
        bin.close();
    }
     
    if (optXml)
    {
        if (optVerbose)
            cout << "writing xml: " << outputDir << name << ".xml" << endl;

        cachedPackers.clear();

        FindPackers(outputDir, namePrefix, "xml", cachedPackers);

        ofstream xml(outputDir + name + ".xml");
        xml << "<atlas>" << endl;
        xml << "\t<trim>" << (optTrim ? "true" : "false") << "</trim>" << endl;
        xml << "\t<rotate>" << (optRotate ? "true" : "false") << "</trim>" << endl;
        for (size_t i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream xmlCache(cachedPackers[i]);
            xml << xmlCache.rdbuf();
            xmlCache.close();
        }
        xml << "</atlas>";
        xml.close();
    }

    if (optJson)
    {
        if (optVerbose)
            cout << "writing json: " << outputDir << name << ".json" << endl;

        cachedPackers.clear();

        FindPackers(outputDir, namePrefix, "json", cachedPackers);

        ofstream json(outputDir + name + ".json");
        json << '{' << endl;
        json << "\t\"trim\":" << (optTrim ? "true" : "false") << ',' << endl;
        json << "\t\"rotate\":" << (optRotate ? "true" : "false") << ',' << endl;
        json << "\t\"textures\":[" << endl;
        for (size_t i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream jsonCache(cachedPackers[i]);
            json << jsonCache.rdbuf();
            jsonCache.close();
            if (i + 1 < cachedPackers.size())
                json << ',';
            json << endl;
        }
        json << "\t]" << endl;
        json << '}';
        json.close();
    }

    StopTimer("saving atlas");

    StopTimer("total");

    WriteAllTimers();

    return EXIT_SUCCESS;
}
