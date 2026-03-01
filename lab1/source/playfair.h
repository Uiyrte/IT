#ifndef PLAYFAIR_H
#define PLAYFAIR_H

#include <string>
#include <vector>
#include <array>

class Playfair {
public:
    Playfair();
    std::string encrypt(const std::string& plaintext, const std::string& key1, const std::string& key2, const std::string& key3, const std::string& key4);
    std::string decrypt(const std::string& ciphertext, const std::string& key1, const std::string& key2, const std::string& key3, const std::string& key4);
    
private:
    void createStandardMatrix(char matrix[5][5]);
    void createKeyMatrix(const std::string& key, char matrix[5][5]);
    void findPosition(char matrix[5][5], char ch, int& row, int& col);
    std::string encryptPair(char a, char b, char key1[5][5], char key2[5][5], char key3[5][5], char key4[5][5]);
    std::string decryptPair(char a, char b, char key1[5][5], char key2[5][5], char key3[5][5], char key4[5][5]);
    std::string prepareText(const std::string& text);
    std::string filterEnglish(const std::string& text);
};

#endif
