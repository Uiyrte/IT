#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

long long gcd(long long a, long long b) {
    while (b) { a %= b; std::swap(a, b); }
    return a;
}

long long extended_gcd(long long a, long long b, long long &x, long long &y) {
    if (b == 0) { x = 1; y = 0; return a; }
    long long x1, y1;
    long long g = extended_gcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

long long mod_inverse(long long a, long long m) {
    long long x, y;
    long long g = extended_gcd(a, m, x, y);
    if (g != 1) throw std::runtime_error("Обратный элемент не существует");
    return (x % m + m) % m;
}

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

bool is_prime(long long n) {
    if (n < 2)       return false;
    if (n == 2)      return true;
    if (n % 2 == 0)  return false;
    for (long long i = 3; i * i <= n; i += 2)
        if (n % i == 0) return false;
    return true;
}

int do_encrypt() {
    std::cout << "=== RSA Шифрование ===" << std::endl;

    long long p, q, kc;

    std::cout << "\nВведите простое число p: ";
    std::cin >> p;
    if (!is_prime(p)) {
        std::cerr << "Ошибка: p = " << p << " не является простым числом." << std::endl;
        return 1;
    }

    std::cout << "Введите простое число q: ";
    std::cin >> q;
    if (!is_prime(q)) {
        std::cerr << "Ошибка: q = " << q << " не является простым числом." << std::endl;
        return 1;
    }
    if (p == q) {
        std::cerr << "Ошибка: p и q должны быть различными простыми числами." << std::endl;
        return 1;
    }

    long long r = p * q;
    if (r <= 255) {
        std::cerr << "Ошибка: r = p*q = " << r << " должно быть больше 255." << std::endl;
        return 1;
    }
    if (r > 65535) {
        std::cerr << "Ошибка: r = p*q = " << r << " превышает 65535." << std::endl;
        return 1;
    }

    long long euler = (p - 1) * (q - 1);

    std::cout << "Введите закрытый ключ KC (1 < KC < " << euler << ", gcd(KC, φ(r)) = 1): ";
    std::cin >> kc;

    if (kc <= 1 || kc >= euler) {
        std::cerr << "Ошибка: KC должен удовлетворять 1 < KC < " << euler << "." << std::endl;
        return 1;
    }
    if (gcd(kc, euler) != 1) {
        std::cerr << "Ошибка: gcd(KC, φ(r)) ≠ 1. KC и φ(r) должны быть взаимно простыми." << std::endl;
        return 1;
    }

    long long ko = mod_inverse(kc, euler);

    std::cout << "\n--- Параметры RSA ---" << std::endl;
    std::cout << "  p          = " << p << std::endl;
    std::cout << "  q          = " << q << std::endl;
    std::cout << "  r = p*q    = " << r << std::endl;
    std::cout << "  φ(r)       = " << euler << std::endl;
    std::cout << "  KC (закр.) = " << kc << std::endl;
    std::cout << "  KO (откр.) = " << ko << std::endl;
    std::cout << "  KC * KO mod φ(r) = " << (kc * ko % euler) << std::endl;

    std::string input_file;
    std::cout << "\nВведите имя входного файла: ";
    std::cin >> input_file;

    std::ifstream fin(input_file, std::ios::binary);
    if (!fin) {
        std::cerr << "Ошибка: не удалось открыть файл \"" << input_file << "\"." << std::endl;
        return 1;
    }

    std::string output_file = input_file + ".enc";
    std::ofstream fout(output_file, std::ios::binary);
    if (!fout) {
        std::cerr << "Ошибка: не удалось создать файл \"" << output_file << "\"." << std::endl;
        return 1;
    }

    std::cout << "\n--- Зашифрованные байты ---" << std::endl;

    std::vector<uint16_t> encrypted;
    unsigned char byte;
    while (fin.read(reinterpret_cast<char*>(&byte), 1)) {
        long long M  = static_cast<long long>(byte);
        long long C  = power_mod(M, ko, r);
        uint16_t c16 = static_cast<uint16_t>(C);
        encrypted.push_back(c16);
        char le[2] = { char(c16 & 0xFF), char((c16 >> 8) & 0xFF) };
        fout.write(le, 2);
    }
    fin.close();
    fout.close();

    for (size_t i = 0; i < encrypted.size(); ++i) {
        std::cout << encrypted[i];
        if (i + 1 < encrypted.size()) std::cout << " ";
        if ((i + 1) % 16 == 0) std::cout << std::endl;
    }
    std::cout << std::endl;

    std::cout << "\nФайл сохранён: " << output_file << std::endl;
    std::cout << "Байт зашифровано: " << encrypted.size() << std::endl;

    return 0;
}

int do_decrypt() {
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

int main() {
    std::cout << "==============================" << std::endl;
    std::cout << "       RSA Утилита            " << std::endl;
    std::cout << "==============================" << std::endl;
    std::cout << "Выберите режим:" << std::endl;
    std::cout << "  1 - Шифрование" << std::endl;
    std::cout << "  2 - Расшифрование" << std::endl;
    std::cout << "Ваш выбор: ";

    int choice;
    std::cin >> choice;

    switch (choice) {
        case 1: return do_encrypt();
        case 2: return do_decrypt();
        default:
            std::cerr << "Ошибка: неверный выбор." << std::endl;
            return 1;
    }
}
