#ifndef str_hpp
#define str_hpp

#include <string>
using namespace std;

#if defined _MSC_VER || defined __MINGW32__
#include <locale>
#include <codecvt>
wstring StrToFile(const string& str);
string FileToStr(const wstring& str);
#else
const string& StrToFile(const string& str);
const string& FileToStr(const string& str);
#endif

#endif
