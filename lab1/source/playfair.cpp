#include "playfair.h"
#include <algorithm>
#include <cctype>
#include <set>

Playfair::Playfair() {}

std::string Playfair::filterEnglish(const std::string& text) {
    std::string result;
    for (char ch : text) {
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            char upper = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            if (upper == 'J') {
                upper = 'I';
            }
            result += upper;
        }
    }
    return result;
}

std::string Playfair::prepareText(const std::string& text) {
    std::string filtered = filterEnglish(text);
    
    if (filtered.length() % 2 != 0) {
        filtered += 'X';
    }
    
    return filtered;
}

void Playfair::createStandardMatrix(char matrix[5][5]) {
    std::string alphabet = "ABCDEFGHIKLMNOPQRSTUVWXYZ";
    
    int idx = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            matrix[i][j] = alphabet[idx++];
        }
    }
}

void Playfair::createKeyMatrix(const std::string& key, char matrix[5][5]) {
    std::string cleanKey = filterEnglish(key);
    std::set<char> used;
    std::string matrixString;
    
    for (char ch : cleanKey) {
        if (used.find(ch) == used.end()) {
            used.insert(ch);
            matrixString += ch;
        }
    }
    
    for (char c = 'A'; c <= 'Z'; c++) {
        if (c == 'J') continue;
        if (used.find(c) == used.end()) {
            matrixString += c;
        }
    }
    
    int idx = 0;
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            matrix[i][j] = matrixString[idx++];
        }
    }
}

void Playfair::findPosition(char matrix[5][5], char ch, int& row, int& col) {
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            if (matrix[i][j] == ch) {
                row = i;
                col = j;
                return;
            }
        }
    }
}

std::string Playfair::encryptPair(char a, char b, char key1[5][5], char key2[5][5], char key3[5][5], char key4[5][5]) {
    int row1, col1, row2, col2;

    findPosition(key1, a, row1, col1);
    findPosition(key4, b, row2, col2);

    std::string result;
    result += key2[row1][col2];
    result += key3[row2][col1];
    return result;
}

std::string Playfair::decryptPair(char a, char b, char key1[5][5], char key2[5][5], char key3[5][5], char key4[5][5]) {
    int row1, col2, row2, col1;

    findPosition(key2, a, row1, col2);
    findPosition(key3, b, row2, col1);

    std::string result;
    result += key1[row1][col1];
    result += key4[row2][col2];
    return result;
}

std::string Playfair::encrypt(const std::string& plaintext, const std::string& key1, const std::string& key2, const std::string& key3, const std::string& key4) {
    char matrices[4][5][5];
    createKeyMatrix(key1, matrices[0]);
    createKeyMatrix(key2, matrices[1]);
    createKeyMatrix(key3, matrices[2]);
    createKeyMatrix(key4, matrices[3]);

    std::string result = plaintext;
    std::vector<size_t> letterPositions;
    std::string letters;

    for (size_t i = 0; i < plaintext.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(plaintext[i]);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            char upper = static_cast<char>(std::toupper(ch));
            if (upper == 'J') {
                upper = 'I';
            }
            letterPositions.push_back(i);
            letters += upper;
        }
    }

    if (letters.empty()) {
        return result;
    }

    bool needsPadding = (letters.length() % 2 != 0);
    
    if (needsPadding) {
        letters += 'X';
    }

    std::string transformed;
    transformed.reserve(letters.size());
    for (size_t i = 0; i < letters.length(); i += 2) {
        transformed += encryptPair(letters[i], letters[i + 1], matrices[0], matrices[1], matrices[2], matrices[3]);
    }

    size_t transformedSize = needsPadding ? transformed.size() - 1 : transformed.size();
    for (size_t i = 0; i < transformedSize; ++i) {
        result[letterPositions[i]] = transformed[i];
    }
    
    if (needsPadding) {
        size_t lastLetterPos = letterPositions[letterPositions.size() - 1];
        result.insert(lastLetterPos + 1, 1, transformed[transformed.size() - 1]);
    }

    return result;
}

std::string Playfair::decrypt(const std::string& ciphertext, const std::string& key1, const std::string& key2, const std::string& key3, const std::string& key4) {
    char matrices[4][5][5];
    createKeyMatrix(key1, matrices[0]);
    createKeyMatrix(key2, matrices[1]);
    createKeyMatrix(key3, matrices[2]);
    createKeyMatrix(key4, matrices[3]);

    std::string result = ciphertext;
    std::vector<size_t> letterPositions;
    std::string letters;

    for (size_t i = 0; i < ciphertext.size(); ++i) {
        unsigned char ch = static_cast<unsigned char>(ciphertext[i]);
        if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
            char upper = static_cast<char>(std::toupper(ch));
            if (upper == 'J') {
                upper = 'I';
            }
            letterPositions.push_back(i);
            letters += upper;
        }
    }

    if (letters.empty()) {
        return result;
    }

    bool needsPadding = (letters.length() % 2 != 0);
    
    if (needsPadding) {
        letters += 'X';
    }

    std::string transformed;
    transformed.reserve(letters.size());
    for (size_t i = 0; i < letters.length(); i += 2) {
        transformed += decryptPair(letters[i], letters[i + 1], matrices[0], matrices[1], matrices[2], matrices[3]);
    }

    size_t transformedSize = needsPadding ? transformed.size() - 1 : transformed.size();
    for (size_t i = 0; i < transformedSize; ++i) {
        result[letterPositions[i]] = transformed[i];
    }
    
    if (needsPadding) {
        size_t lastLetterPos = letterPositions[letterPositions.size() - 1];
        result.insert(lastLetterPos + 1, 1, transformed[transformed.size() - 1]);
    }

    return result;
}
