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

#include "binary.hpp"
#include <iostream>

void WriteString(ofstream& bin, const string& value)
{
    bin.write(value.data(), value.length() + 1);
}

void WriteShort(ofstream& bin, int16_t value)
{
    bin.put(static_cast<uint8_t>(value & 0xff));
    bin.put(static_cast<uint8_t>((value >> 8) & 0xff));
}

void WriteByte(ofstream& bin, char value)
{
    bin.write(&value, 1);
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
