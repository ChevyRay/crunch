#include "binary.hpp"
#include <iostream>

void WriteString(ofstream& bin, const string& value)
{
    bin.write(value.data(), value.length() + 1);
}

void WriteShort(ofstream& bin, int16_t value)
{
    bin.write(reinterpret_cast<char*>(&value), 2);
}

string ReadString(ifstream& bin)
{
    char data[256];
    bin.read(data, 1);
    char* chr = data;
    while (*chr != '\0')
        bin.read(++chr, 1);
    return data;
}

int16_t ReadShort(ifstream& bin)
{
    int16_t value;
    bin.read(reinterpret_cast<char*>(&value), 2);
    return value;
}
