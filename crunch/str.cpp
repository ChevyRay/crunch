#include "str.hpp"

#if defined _MSC_VER || defined __MINGW32__
#include <locale>
#include <codecvt>
wstring StrToFile(const string& str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}
string FileToStr(const wstring& str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(str);
}
#else
const string& StrToFile(const string& str)
{
    return str;
}
const string& FileToStr(const string& str)
{
    return str;
}
#endif
