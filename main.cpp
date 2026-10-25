#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS // В задании требуются стандартные функции fopen/fgets/fputs.
#endif

#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#endif

constexpr int MAX_LENGTH = 4096;

// Вариант 1: массив символов. Конец строки задаётся длиной, а не '\0'.
bool processArray() {
    char text[MAX_LENGTH];
    int length = 0;
    bool tooLong = false;

    std::printf("Введите строку (не более %d байт):\n", MAX_LENGTH);
    int ch;
    while ((ch = std::getchar()) != '\n' && ch != EOF) {
        if (length < MAX_LENGTH) {
            text[length++] = static_cast<char>(ch);
        } else {
            tooLong = true;
        }
    }

    if (tooLong || std::ferror(stdin)) {
        std::fprintf(stderr, "Ошибка: строка слишком длинная или не удалось прочитать ввод.\n");
        return false;
    }

    int read = 0;
    int write = 0;
    int maxSpaces = 0;

    while (read < length) {
        if (text[read] != ' ') {
            text[write++] = text[read++];
            continue;
        }

        int spaces = 0;
        while (read < length && text[read] == ' ') {
            ++spaces;
            ++read;
        }
        if (spaces > maxSpaces) {
            maxSpaces = spaces;
        }
        if (spaces % 3 == 0) {
            text[write++] = '*';
        }
    }
    length = write;

    std::printf("Максимальное количество подряд идущих пробелов: %d\n", maxSpaces);
    std::printf("Результат: [");
    std::fwrite(text, sizeof(char), length, stdout);
    std::printf("]\n");
    return true;
}

// Вариант 2: Cи-строка. Обход продолжается до нулевого символа.
bool processCString() {
    // Два места сверх лимита нужны для '\n' и '\0' при чтении fgets().
    char text[MAX_LENGTH + 2];
    std::printf("Введите строку (не более %d байт):\n", MAX_LENGTH);

    if (std::fgets(text, sizeof(text), stdin) == nullptr) {
        std::fprintf(stderr, "Ошибка: не удалось прочитать строку.\n");
        return false;
    }

    const std::size_t length = std::strcspn(text, "\r\n");
    if (length > MAX_LENGTH) {
        std::fprintf(stderr, "Ошибка: строка слишком длинная.\n");
        return false;
    }
    text[length] = '\0';

    int read = 0;
    int write = 0;
    int maxSpaces = 0;

    while (text[read] != '\0') {
        if (text[read] != ' ') {
            text[write++] = text[read++];
            continue;
        }

        int spaces = 0;
        while (text[read] == ' ') {
            ++spaces;
            ++read;
        }
        if (spaces > maxSpaces) {
            maxSpaces = spaces;
        }
        if (spaces % 3 == 0) {
            text[write++] = '*';
        }
    }
    text[write] = '\0';

    std::printf("Максимальное количество подряд идущих пробелов: %d\n", maxSpaces);
    std::printf("Результат: [%s]\n", text);
    return true;
}

// Вариант 3: чтение одной строки из input.txt и запись в output.txt.
bool processFile() {
    char text[MAX_LENGTH + 2];
    std::FILE* input = std::fopen("input.txt", "r");
    if (input == nullptr) {
        std::perror("input.txt");
        return false;
    }

    if (std::fgets(text, sizeof(text), input) == nullptr) {
        if (std::ferror(input)) {
            std::perror("input.txt");
            std::fclose(input);
            return false;
        }
        text[0] = '\0'; // Пустой файл считаем пустой строкой.
    }
    if (std::fclose(input) != 0) {
        std::perror("input.txt");
        return false;
    }

    const std::size_t length = std::strcspn(text, "\r\n");
    if (length > MAX_LENGTH) {
        std::fprintf(stderr, "Ошибка: строка в input.txt слишком длинная.\n");
        return false;
    }
    text[length] = '\0';

    int read = 0;
    int write = 0;
    int maxSpaces = 0;

    while (text[read] != '\0') {
        if (text[read] != ' ') {
            text[write++] = text[read++];
            continue;
        }

        int spaces = 0;
        while (text[read] == ' ') {
            ++spaces;
            ++read;
        }
        if (spaces > maxSpaces) {
            maxSpaces = spaces;
        }
        if (spaces % 3 == 0) {
            text[write++] = '*';
        }
    }
    text[write] = '\0';

    std::FILE* output = std::fopen("output.txt", "w");
    if (output == nullptr) {
        std::perror("output.txt");
        return false;
    }
    const bool written = std::fputs(text, output) != EOF;
    const bool closed = std::fclose(output) == 0;
    if (!written || !closed) {
        std::fprintf(stderr, "Ошибка: не удалось записать output.txt.\n");
        return false;
    }

    std::printf("Максимальное количество подряд идущих пробелов: %d\n", maxSpaces);
    std::printf("Результат: [%s]\n", text);
    std::printf("Результат записан в output.txt.\n");
    return true;
}

int main() {
#ifdef _WIN32
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    std::printf("Лабораторная работа №1. Обработка строки символов\n");
    std::printf("1 — массив символов\n2 — Cи-строка\n3 — строка в файле\n");
    std::printf("Выберите вариант (1–3): ");

    // Читаем пункт меню целиком, чтобы остаток ввода не стал частью строки.
    const int choice = std::getchar();
    int ch = std::getchar();
    bool validChoice = choice >= '1' && choice <= '3';
    while (ch != '\n' && ch != EOF) {
        if (ch != ' ' && ch != '\t' && ch != '\r') {
            validChoice = false;
        }
        ch = std::getchar();
    }
    if (!validChoice) {
        std::fprintf(stderr, "Ошибка: выберите 1, 2 или 3.\n");
        return 1;
    }

    switch (choice) {
        case '1': return processArray() ? 0 : 1;
        case '2': return processCString() ? 0 : 1;
        case '3': return processFile() ? 0 : 1;
        default: return 1;
    }
}
