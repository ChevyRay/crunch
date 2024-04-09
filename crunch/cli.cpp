#include "cli.hpp"

#include <iostream>
#include <string>

#include "options.hpp"

using namespace std;
using namespace rbp;

const static string expectedSize = "4096, 2048, 1024, 512, 256, 128, or 64",
                    expectedPaddingOrStretch = "integer from 0 to 16",
                    expectedBinaryStringFormat = "0, 16 or 7";

void PrintHelp(int argc, const char *argv[])
{
    if (argc >= 3)
        return;

    if (argc == 2)
    {
        string arg = argv[1];
        if (arg == "-h" || arg == "-?" || arg == "--help")
        {
            cout << helpMessage << endl;
            exit(EXIT_SUCCESS);
        }
        else if (arg == "--version")
        {
            cout << "crunch " << version << endl;
            exit(EXIT_SUCCESS);
        }
    }

    cerr << "invalid input, expected: \"crunch [OUTPUT] [INPUT1,INPUT2,INPUT3...] [OPTIONS...]\"" << endl;

    exit(EXIT_FAILURE);
}

static int GetPackSize(const string &str)
{
    for (int i = 64; i <= 4096; i *= 2)
        if (str == to_string(i))
            return i;
    cerr << "invalid size: " << str << endl;
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

static int GetStretch(const string &str)
{
    for (int i = 0; i <= 16; ++i)
        if (str == to_string(i))
            return i;
    cerr << "invalid stretch value: " << str << endl;
    exit(EXIT_FAILURE);
    return 0;
}

static BinaryStringFormat GetBinaryStringFormat(const string &str)
{
    if (str == "0")
        return BinaryStringFormat::NullTerminated;
    if (str == "16")
        return BinaryStringFormat::Prefix16;
    if (str == "7")
        return BinaryStringFormat::Prefix7;

    cerr << "invalid binary string format: " << str << endl;
    exit(EXIT_FAILURE);
}

static MaxRectsBinPack::FreeRectChoiceHeuristic GetChoiceHeuristic(const string &str)
{
    if (str == "bssf")
        return MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestShortSideFit;
    if (str == "blsf")
        return MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestLongSideFit;
    if (str == "baf")
        return MaxRectsBinPack::FreeRectChoiceHeuristic::RectBestAreaFit;
    if (str == "blr")
        return MaxRectsBinPack::FreeRectChoiceHeuristic::RectBottomLeftRule;
    if (str == "cpr")
        return MaxRectsBinPack::FreeRectChoiceHeuristic::RectContactPointRule;

    cerr << "invalid heuristic: " << str << endl;
    exit(EXIT_FAILURE);
}

static void PrintNoArgument(const string &expected, const string &argument)
{
    cerr << "expected " << expected << " for argument " << argument << endl;
    exit(EXIT_FAILURE);
}

void ParseArguments(int argc, const char *argv[], int offset)
{
    int width = -1, height = -1;

    for (int i = offset; i < argc; ++i)
    {
        string arg = argv[i], nextArg;

        bool noArgumentAhead = i == argc - 1;
        if (!noArgumentAhead)
            nextArg = argv[i + 1];

        if (arg == "--default" || arg == "-d")
            options.xml = options.premultiply = options.trim = options.unique = true;

        // ================================================================

        else if (arg == "--xml" || arg == "-x")
            options.xml = true;
        else if (arg == "--json" || arg == "-j")
            options.json = true;
        else if (arg == "--binary" || arg == "-b")
            options.binary = true;

        // ================================================================

        else if (arg == "--size" || arg == "-s")
        {
            if (noArgumentAhead)
                PrintNoArgument(expectedSize, arg);
            options.width = options.height = GetPackSize(nextArg);
            i++;
        }
        else if (arg == "--width" || arg == "-w")
        {
            if (noArgumentAhead)
                PrintNoArgument(expectedSize, arg);
            width = GetPackSize(nextArg);
            i++;
        }
        else if (arg == "--height" || arg == "-h")
        {
            if (noArgumentAhead)
                PrintNoArgument(expectedSize, arg);
            height = GetPackSize(nextArg);
            i++;
        }
        else if (arg == "--padding" || arg == "-pd")
        {
            if (noArgumentAhead)
                PrintNoArgument(expectedPaddingOrStretch, arg);
            options.padding = GetPadding(nextArg);
            i++;
        }
        else if (arg == "--stretch" || arg == "-st")
        {
            if (noArgumentAhead)
                PrintNoArgument(expectedPaddingOrStretch, arg);
            options.stretch = GetStretch(nextArg);
            i++;
        }

        // ================================================================

        else if (arg == "--premultiply" || arg == "-p")
            options.premultiply = true;
        else if (arg == "--unique" || arg == "-u")
            options.unique = true;
        else if (arg == "--trim" || arg == "-t")
            options.trim = true;
        else if (arg == "--rotate" || arg == "-r")
            options.rotate = true;
        else if (arg == "--heuristic" || arg == "-hr")
        {
            if (noArgumentAhead)
                PrintNoArgument(expectedBinaryStringFormat, arg);
            options.choiceHeuristic = GetChoiceHeuristic(nextArg);
            i++;
        }

        // ================================================================

        else if (arg == "--binstr" || arg == "-bs")
        {
            if (noArgumentAhead)
                PrintNoArgument(expectedBinaryStringFormat, arg);
            options.binaryStringFormat = GetBinaryStringFormat(nextArg);
            i++;
        }
        else if (arg == "--force" || arg == "-f")
            options.force = true;
        else if (arg == "--verbose" || arg == "-v")
            options.verbose = true;
        else if (arg == "--time" || arg == "-tm")
            options.useTimeForHash = true;
        else if (arg == "--split" || arg == "-sp")
            options.splitSubdirectories = true;
        else if (arg == "--nozero" || arg == "-nz")
            options.noZero = true;
        else
        {
            cerr << "unexpected argument: " << arg << endl;
            exit(EXIT_FAILURE);
        }
    }

    if (width != -1)
        options.width = width;

    if (height != -1)
        options.height = height;

    if (options.verbose)
    {
        cout << "options..." << endl;
        cout << "\t--xml: " << (options.xml ? "true" : "false") << endl;
        cout << "\t--json: " << (options.json ? "true" : "false") << endl;
        cout << "\t--binary: " << (options.binary ? "true" : "false") << endl;

        if (options.width == options.height)
            cout << "\t--size: " << options.width << endl;
        else
        {
            cout << "\t--width: " << options.width << endl;
            cout << "\t--height: " << options.height << endl;
        }
        cout << "\t--padding: " << options.padding << endl;

        cout << "\t--premultiply: " << (options.premultiply ? "true" : "false") << endl;
        cout << "\t--unique: " << (options.unique ? "true" : "false") << endl;
        cout << "\t--trim: " << (options.trim ? "true" : "false") << endl;
        cout << "\t--rotate: " << (options.rotate ? "true" : "false") << endl;

        cout << "\t--binstr: " << (options.binaryStringFormat == BinaryStringFormat::NullTerminated ? "0" : (options.binaryStringFormat == BinaryStringFormat::Prefix16 ? "16" : "7")) << endl;
        cout << "\t--force: " << (options.force ? "true" : "false") << endl;
        cout << "\t--verbose: " << (options.verbose ? "true" : "false") << endl;
        cout << "\t--time: " << (options.useTimeForHash ? "true" : "false") << endl;
        cout << "\t--split: " << (options.splitSubdirectories ? "true" : "false") << endl;
        cout << "\t--nozero: " << (options.noZero ? "true" : "false") << endl;
    }
}
