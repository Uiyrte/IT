#include "vigenere.h"
#include <algorithm>
#include <cwctype>

Vigenere::Vigenere() {}

std::wstring Vigenere::toWString(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.from_bytes(str);
}

std::string Vigenere::toString(const std::wstring& wstr) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
    return converter.to_bytes(wstr);
}

std::wstring Vigenere::filterRussian(const std::wstring& text) {
    std::wstring result;
    for (wchar_t ch : text) {
        if ((ch >= L'А' && ch <= L'Я') || (ch >= L'а' && ch <= L'я') || ch == L'Ё' || ch == L'ё') {
            result += std::towupper(ch);
        }
    }
    return result;
}

int Vigenere::charToIndex(wchar_t ch) {
    ch = std::towupper(ch);
    if (ch == L'Ё') return 6;
    if (ch >= L'А' && ch <= L'Е') return ch - L'А';
    if (ch >= L'Ж' && ch <= L'Я') return ch - L'А' + 1;
    return 0;
}

wchar_t Vigenere::indexToChar(int index, bool uppercase) {
    index = index % RUSSIAN_ALPHABET_SIZE;
    if (index < 0) index += RUSSIAN_ALPHABET_SIZE;
    
    if (index == 6) return uppercase ? L'Ё' : L'ё';
    
    wchar_t result;
    if (index < 6) {
        result = L'А' + index;
    } else {
        result = L'Ж' + (index - 7);
    }
    
    if (!uppercase) {
        result = std::towlower(result);
    }
    
    return result;
}

std::string Vigenere::encrypt(const std::string& plaintext, const std::string& key) {
    std::wstring wplaintext = toWString(plaintext);
    std::wstring wkey = toWString(key);
    
    std::wstring keyFiltered = filterRussian(wkey);
    
    if (keyFiltered.empty()) {
        return plaintext;
    }
    
    std::wstring result;
    size_t keyIndex = 0;
    
    for (wchar_t ch : wplaintext) {
        if ((ch >= L'А' && ch <= L'Я') || (ch >= L'а' && ch <= L'я') || ch == L'Ё' || ch == L'ё') {
            int plainIndex = charToIndex(ch);
            int keyCharIndex = charToIndex(keyFiltered[keyIndex % keyFiltered.length()]);
            
            int encryptedIndex = (plainIndex + keyCharIndex) % RUSSIAN_ALPHABET_SIZE;
            result += indexToChar(encryptedIndex, std::isupper(ch));
            
            keyIndex++;
        } else {
            result += ch;
        }
    }
    
    return toString(result);
}

std::string Vigenere::decrypt(const std::string& ciphertext, const std::string& key) {
    std::wstring wciphertext = toWString(ciphertext);
    std::wstring wkey = toWString(key);
    
    std::wstring keyFiltered = filterRussian(wkey);
    
    if (keyFiltered.empty()) {
        return ciphertext;
    }
    
    std::wstring result;
    size_t keyIndex = 0;
    
    for (wchar_t ch : wciphertext) {
        if ((ch >= L'А' && ch <= L'Я') || (ch >= L'а' && ch <= L'я') || ch == L'Ё' || ch == L'ё') {
            int cipherIndex = charToIndex(ch);
            int keyCharIndex = charToIndex(keyFiltered[keyIndex % keyFiltered.length()]);
            
            int decryptedIndex = (cipherIndex - keyCharIndex + RUSSIAN_ALPHABET_SIZE) % RUSSIAN_ALPHABET_SIZE;
            result += indexToChar(decryptedIndex, std::isupper(ch));
            
            keyIndex++;
        } else {
            result += ch;
        }
    }
    
    return toString(result);
}
