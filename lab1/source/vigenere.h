#ifndef VIGENERE_H
#define VIGENERE_H

#include <string>
#include <codecvt>
#include <locale>

class Vigenere {
public:
    Vigenere();
    std::string encrypt(const std::string& plaintext, const std::string& key);
    std::string decrypt(const std::string& ciphertext, const std::string& key);
    
private:
    std::wstring toWString(const std::string& str);
    std::string toString(const std::wstring& wstr);
    std::wstring filterRussian(const std::wstring& text);
    static const int RUSSIAN_ALPHABET_SIZE = 33;
    int charToIndex(wchar_t ch);
    wchar_t indexToChar(int index, bool uppercase);
};

#endif
