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

#include "options.hpp"

#include <iostream>
#include <cstdint>

void WriteString(ofstream &bin, const string &value)
{
    switch (options.binaryStringFormat)
    {
    case BinaryStringFormat::NullTerminated:
        WriteStringNullTerminated(bin, value);
        break;
    case BinaryStringFormat::Prefix16:
        WriteStringPrefixed(bin, value);
        break;
    case BinaryStringFormat::Prefix7:
        WriteString7BitPrefixed(bin, value);
        break;
    }
}

void WriteStringNullTerminated(ofstream &bin, const string &value)
{
    bin.write(value.data(), value.length() + 1);
}

void WriteStringPrefixed(ofstream &bin, const string &value)
{
    WriteShort(bin, static_cast<int16_t>(value.length()));
    bin.write(value.data(), value.length());
}

void WriteString7BitPrefixed(ofstream &bin, const string &value)
{
    // Using 7-bit encoding algorithm from dotnet
    // https://github.com/dotnet/runtime/blob/main/src/libraries/System.Private.CoreLib/src/System/IO/BinaryWriter.cs#L473

    int16_t length = static_cast<int16_t>(value.length());
    while (length > 0x7Fu)
    {
        WriteByte(bin, static_cast<uint8_t>(length | ~0x7Fu));
        length >>= 7;
    }

    WriteByte(bin, static_cast<uint8_t>(length));
    bin.write(value.data(), value.length());
}

void WriteShort(ofstream &bin, int16_t value)
{
    bin.put(static_cast<uint8_t>(value & 0xff));
    bin.put(static_cast<uint8_t>((value >> 8) & 0xff));
}

void WriteByte(ofstream &bin, char value)
{
    bin.write(&value, 1);
}

int16_t ReadShort(ifstream &bin)
{
    uint8_t high, low;
    bin.read(reinterpret_cast<char *>(&low), 1);
    bin.read(reinterpret_cast<char *>(&high), 1);
    return high << 8 | low;
}
