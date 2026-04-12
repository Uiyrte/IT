#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>

long long power_mod(long long base, long long exp, long long mod) {
    long long result = 1;
    long long b = base % mod;
    while (exp > 0) {
        if (exp % 2 != 0) {
            result = result * b % mod;
            exp -= 1;
        } else {
            b = b * b % mod;
            exp /= 2;
        }
    }
    return result;
}

int main() {
    std::cout << "=== RSA Расшифрование ===" << std::endl;

    long long r, kc;

    std::cout << "\nВведите модуль r (= p*q): ";
    std::cin >> r;
    if (r <= 255 || r > 65535) {
        std::cerr << "Ошибка: r должно быть в диапазоне (255, 65535]." << std::endl;
        return 1;
    }

    std::cout << "Введите закрытый ключ KC: ";
    std::cin >> kc;
    if (kc <= 0) {
        std::cerr << "Ошибка: KC должен быть положительным числом." << std::endl;
        return 1;
    }

    std::string input_file;
    std::cout << "\nВведите имя зашифрованного файла: ";
    std::cin >> input_file;

    std::ifstream fin(input_file, std::ios::binary);
    if (!fin) {
        std::cerr << "Ошибка: не удалось открыть файл \"" << input_file << "\"." << std::endl;
        return 1;
    }

    std::string output_file = input_file + ".dec";
    std::ofstream fout(output_file, std::ios::binary);
    if (!fout) {
        std::cerr << "Ошибка: не удалось создать файл \"" << output_file << "\"." << std::endl;
        return 1;
    }

    std::cout << "\n--- Расшифрованные байты ---" << std::endl;

    std::vector<unsigned char> decrypted;
    char le[2];
    while (fin.read(le, 2)) {
        uint16_t c16 = uint16_t(uint8_t(le[0])) | (uint16_t(uint8_t(le[1])) << 8);
        long long C  = static_cast<long long>(c16);
        long long M  = power_mod(C, kc, r);

        if (M > 255)
            std::cerr << "Предупреждение: значение " << M << " выходит за [0..255]." << std::endl;

        unsigned char byte = static_cast<unsigned char>(M & 0xFF);
        decrypted.push_back(byte);
        fout.write(reinterpret_cast<const char*>(&byte), 1);
    }
    fin.close();
    fout.close();

    for (size_t i = 0; i < decrypted.size(); ++i) {
        std::cout << static_cast<int>(decrypted[i]);
        if (i + 1 < decrypted.size()) std::cout << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    std::cout << std::endl;

    std::cout << "\nФайл сохранён: " << output_file << std::endl;
    std::cout << "Байт расшифровано: " << decrypted.size() << std::endl;

    return 0;
}
