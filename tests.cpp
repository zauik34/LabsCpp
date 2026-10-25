#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

std::string readFile(const char* path) {
    std::ifstream file(path, std::ios::binary);
    REQUIRE(file.is_open());
    return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
}

void checkString(int mode, const char* source, const char* expected, int maximum) {
    CAPTURE(mode, source);
    std::filesystem::create_directories("build/test-data");
    {
        std::ofstream input("build/test-data/input.txt");
        REQUIRE(input.is_open());
        input << source << '\n';
    }
    {
        std::ofstream input("build/test-data/stdin.txt");
        REQUIRE(input.is_open());
        input << mode << '\n';
        if (mode != 3) {
            input << source << '\n';
        }
    }

#ifdef _WIN32
    const int code = std::system("cd build\\test-data && ..\\..\\lab1.exe < stdin.txt > result.txt");
#else
    const int code = std::system("cd build/test-data && ../../lab1 < stdin.txt > result.txt");
#endif
    REQUIRE(code == 0);

    const std::string report = readFile("build/test-data/result.txt");
    CHECK(report.find(std::string("Результат: [") + expected + "]") != std::string::npos);
    const std::string maximumLine = "пробелов: " + std::to_string(maximum);
    const bool foundMaximum = report.find(maximumLine + "\r\n") != std::string::npos ||
                              report.find(maximumLine + "\n") != std::string::npos;
    CHECK(foundMaximum);
    if (mode == 3) {
        CHECK(readFile("build/test-data/output.txt") == expected);
    }
}

TEST_CASE("Replace runs divisible by three and remove other spaces") {
    for (int mode = 1; mode <= 3; ++mode) {
        checkString(mode, "", "", 0);
        checkString(mode, "abc", "abc", 0);
        checkString(mode, "A   B      C", "A*B*C", 6);
        checkString(mode, "A B  C    D", "ABCD", 4);
        checkString(mode, "A   B  C      D", "A*BC*D", 6);
        checkString(mode, "A   B    C", "A*BC", 4);
        checkString(mode, "   abc  ", "*abc", 3);
    }
}
