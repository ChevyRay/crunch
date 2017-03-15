#ifndef binary_hpp
#define binary_hpp

#include <fstream>
#include <string>

using namespace std;

void WriteString(ofstream& bin, const string& value);
void WriteShort(ofstream& bin, int16_t value);
string ReadString(ifstream& bin);
int16_t ReadShort(ifstream& bin);

#endif
