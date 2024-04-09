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

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>

#include "binary.hpp"
#include "bitmap.hpp"
#include "cli.hpp"
#include "hash.hpp"
#include "options.hpp"
#include "packer.hpp"

#define EXIT_SKIPPED 2

using namespace std;
namespace fs = std::filesystem;

const int binVersion = 0;

static string NormalizePath(const string &path)
{
    string str = path;
    replace(str.begin(), str.end(), '\\', '/');
    return str;
}

static void LoadBitmap(const string &path, const string &name, vector<Bitmap *> &bitmaps)
{
    if (options.verbose)
        cout << '\t' << path << endl;

    bitmaps.push_back(new Bitmap(path, name, options.premultiply, options.trim));
}

static void LoadBitmaps(const string &root, const string &prefix, vector<Bitmap *> &bitmaps)
{
    for (const auto &entry : fs::directory_iterator(root))
    {
        fs::path path = entry.path();
        string pathName = path.string();

        if (entry.is_directory())
            LoadBitmaps(pathName, prefix + path.filename().string() + '/', bitmaps);
        else if (path.extension().string() == ".png")
            LoadBitmap(pathName, NormalizePath(prefix + path.stem().string()), bitmaps);
    }
}

static void FindPackers(const string &root, const string &name, const string &ext, vector<string> &packers)
{
    for (auto &entry : fs::directory_iterator(root))
    {
        if (entry.is_directory())
            continue;

        fs::path path = entry.path();

        if (path.filename().string().starts_with(name) && path.extension().string() == ext)
            packers.push_back(path.string());
    }
}

static int Pack(uint64_t newHash, string &outputDirectory, string &name, vector<string> &inputs, string prefix = "")
{
    string outputName = name;

    if (!outputDirectory.empty())
        outputName = outputDirectory + '/' + outputName;

    for (auto &input : inputs)
        if (fs::is_directory(input))
            HashFiles(newHash, input, options.useTimeForHash);
        else
            HashFile(newHash, input, options.useTimeForHash);

    // Load the old hash
    uint64_t oldHash;
    if (!options.force && LoadHash(oldHash, outputName + ".hash") && newHash == oldHash)
    {
        if (options.splitSubdirectories)
            return EXIT_SKIPPED;

        cout << "atlas is unchanged: " << name << endl;
        return EXIT_SUCCESS;
    }

    // Remove old files
    fs::remove(outputName + ".hash");
    fs::remove(outputName + ".bin");
    fs::remove(outputName + ".xml");
    fs::remove(outputName + ".json");
    fs::remove(outputName + ".png");
    for (int i = 0; i < 16; ++i)
        fs::remove(outputName + to_string(i) + ".png");

    // Load the bitmaps from all the input files and directories
    if (options.verbose)
        cout << "loading images..." << endl;

    vector<Bitmap *> bitmaps;
    for (auto &input : inputs)
    {
        if (fs::is_directory(input))
            LoadBitmaps(input, prefix, bitmaps);
        else
            LoadBitmap(input, prefix + input, bitmaps);
    }

    // Sort the bitmaps by area
    stable_sort(bitmaps.begin(), bitmaps.end(), [](const Bitmap *a, const Bitmap *b)
                { return (a->width * a->height) < (b->width * b->height); });

    // Pack the bitmaps
    vector<Packer *> packers;
    while (!bitmaps.empty())
    {
        if (options.verbose)
            cout << "packing " << bitmaps.size() << " images..." << endl;

        auto packer = new Packer(options.width, options.height, options.padding, options.stretch);
        packer->Pack(bitmaps, options.unique, options.rotate, options.choiceHeuristic);
        packers.push_back(packer);

        if (options.verbose)
            cout << "finished packing: " << name << (options.noZero && bitmaps.empty() ? "" : to_string(packers.size() - 1)) << " (" << packer->width << " x " << packer->height << ')' << endl;

        if (packer->bitmaps.empty())
        {
            cerr << "packing failed, could not fit bitmap: " << (bitmaps.back())->name << endl;
            return EXIT_FAILURE;
        }
    }

    bool noZero = options.noZero && packers.size() == 1;

    // Save the atlas image
    for (int i = 0; i < packers.size(); ++i)
    {
        string pngName = outputName + (noZero ? "" : to_string(i)) + ".png";
        if (options.verbose)
            cout << "writing png: " << pngName << endl;
        packers[i]->SavePng(pngName);
    }

    // Save the atlas binary
    if (options.binary)
    {
        if (options.verbose)
            cout << "writing bin: " << outputName << ".bin" << endl;

        ofstream bin(outputName + ".bin", ios::binary);

        if (!options.splitSubdirectories)
        {
            WriteByte(bin, 'c');
            WriteByte(bin, 'r');
            WriteByte(bin, 'c');
            WriteByte(bin, 'h');
            WriteShort(bin, binVersion);
            WriteByte(bin, options.trim);
            WriteByte(bin, options.rotate);
            WriteByte(bin, (char)options.binaryStringFormat);
        }
        WriteShort(bin, (int16_t)packers.size());
        for (int i = 0; i < packers.size(); ++i)
            packers[i]->SaveBin(name + (noZero ? "" : to_string(i)), bin, options.trim, options.rotate);
        bin.close();
    }

    // Save the atlas xml
    if (options.xml)
    {
        if (options.verbose)
            cout << "writing xml: " << outputName << ".xml" << endl;

        ofstream xml(outputName + ".xml");
        if (!options.splitSubdirectories)
        {
            xml << "<atlas>" << endl;
            xml << "\t<trim>" << (options.trim ? "true" : "false") << "</trim>" << endl;
            xml << "\t<rotate>" << (options.rotate ? "true" : "false") << "</trim>" << endl;
        }
        for (int i = 0; i < packers.size(); ++i)
            packers[i]->SaveXml(name + (noZero ? "" : to_string(i)), xml, options.trim, options.rotate);
        if (!options.splitSubdirectories)
            xml << "</atlas>" << endl;
        xml.close();
    }

    // Save the atlas json
    if (options.json)
    {
        if (options.verbose)
            cout << "writing json: " << outputName << ".json" << endl;

        ofstream json(outputName + ".json");
        if (!options.splitSubdirectories)
        {
            json << '{' << endl;
            json << "\t\"trim\": " << (options.trim ? "true" : "false") << ',' << endl;
            json << "\t\"rotate\": " << (options.rotate ? "true" : "false") << ',' << endl;
            json << "\t\"textures\": {" << endl;
        }
        for (int i = 0; i < packers.size(); ++i)
        {
            packers[i]->SaveJson(name + (noZero ? "" : to_string(i)), json, options.trim, options.rotate);
            if (!options.splitSubdirectories)
            {
                if (i != packers.size() - 1)
                    json << ',';
                json << endl;
            }
        }
        if (!options.splitSubdirectories)
        {
            json << "\t}" << endl;
            json << '}' << endl;
        }
        json.close();
    }

    // Save the new hash
    SaveHash(newHash, outputName + ".hash");

    return EXIT_SUCCESS;
}

int main(int argc, const char *argv[])
{
    PrintHelp(argc, argv);

    // Get the output directory and name
    fs::path outputPath = NormalizePath(argv[1]);
    string outputDir = outputPath.parent_path().string(), name = outputPath.filename().string();
    string outputName = outputPath.string();

    // Get all the input files and directories
    vector<string> inputs;
    stringstream ss(argv[2]);
    while (ss.good())
    {
        string inputStr;
        getline(ss, inputStr, ',');
        inputs.push_back(NormalizePath(inputStr));
    }

    ParseArguments(argc, argv, 3);

    // Hash the arguments and input directories
    uint64_t newHash = 0;
    for (int i = 1; i < argc; ++i)
        HashString(newHash, argv[i]);

    if (!options.splitSubdirectories)
    {
        int result = Pack(newHash, outputDir, name, inputs);

        if (result != EXIT_SUCCESS)
            return result;

        return EXIT_SUCCESS;
    }

    string newInput, namePrefix;
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

    bool skipped = true;
    for (auto &subdir : fs::directory_iterator(newInput))
    {
        if (!subdir.is_directory())
            continue;

        string newName = subdir.path().filename().string(), prefixedName = namePrefix + newName;
        vector<string> input{subdir.path().string()};
        int result = Pack(newHash, outputDir, prefixedName, input, newName + '/');

        if (result == EXIT_SUCCESS)
            skipped = false;
        else if (result != EXIT_SKIPPED)
            return result;
    }

    if (skipped)
    {
        cout << "atlas is unchanged: " << name << endl;

        return EXIT_SUCCESS;
    }

    fs::remove(outputName + ".bin");
    fs::remove(outputName + ".xml");
    fs::remove(outputName + ".json");

    vector<string> cachedPackers;

    if (options.binary)
    {
        if (options.verbose)
            cout << "writing bin: " << outputName << ".bin" << endl;

        FindPackers(outputDir, namePrefix, ".bin", cachedPackers);

        ofstream bin(outputName + ".bin", ios::binary);
        WriteByte(bin, 'c');
        WriteByte(bin, 'r');
        WriteByte(bin, 'c');
        WriteByte(bin, 'h');
        WriteShort(bin, binVersion);
        WriteByte(bin, options.trim);
        WriteByte(bin, options.rotate);
        WriteByte(bin, (char)options.binaryStringFormat);

        int16_t imageCount = 0;
        for (int i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream binCache(cachedPackers[i], ios::binary);
            imageCount += ReadShort(binCache);
            binCache.close();
        }
        WriteShort(bin, imageCount);
        for (int i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream binCache(cachedPackers[i], ios::binary);
            ReadShort(binCache);
            bin << binCache.rdbuf();
            binCache.close();
        }
        bin.close();
    }

    if (options.xml)
    {
        if (options.verbose)
            cout << "writing xml: " << outputName << ".xml" << endl;

        cachedPackers.clear();

        FindPackers(outputDir, namePrefix, ".xml", cachedPackers);

        ofstream xml(outputName + ".xml");
        xml << "<atlas>" << endl;
        xml << "\t<trim>" << (options.trim ? "true" : "false") << "</trim>" << endl;
        xml << "\t<rotate>" << (options.rotate ? "true" : "false") << "</trim>" << endl;
        for (int i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream xmlCache(cachedPackers[i]);
            xml << xmlCache.rdbuf();
            xmlCache.close();
        }
        xml << "</atlas>" << endl;
        xml.close();
    }

    if (options.json)
    {
        if (options.verbose)
            cout << "writing json: " << outputName << ".json" << endl;

        cachedPackers.clear();

        FindPackers(outputDir, namePrefix, ".json", cachedPackers);

        ofstream json(outputName + ".json");
        json << '{' << endl;
        json << "\t\"trim\": " << (options.trim ? "true" : "false") << ',' << endl;
        json << "\t\"rotate\": " << (options.rotate ? "true" : "false") << ',' << endl;
        json << "\t\"textures\": {" << endl;
        for (int i = 0; i < cachedPackers.size(); ++i)
        {
            ifstream jsonCache(cachedPackers[i]);
            json << jsonCache.rdbuf();
            jsonCache.close();
            if (i != cachedPackers.size() - 1)
                json << ',';
            json << endl;
        }
        json << "\t}" << endl;
        json << '}' << endl;
        json.close();
    }

    return EXIT_SUCCESS;
}
