#ifndef hash_hpp
#define hash_hpp

#include <string>
using namespace std;

template <class T>
void hashCombine(std::size_t& hash, const T& v);
void hashFile(size_t& hash, const string& file);
void hashFiles(size_t& hash, const string& root);
void hashData(size_t& hash, const char* data, size_t size);
bool loadHash(size_t& hash, const string& file);
void saveHash(size_t hash, const string& file);

#endif
