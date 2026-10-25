#define CATCH_CONFIG_MAIN
#include <catch2/catch.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <random>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct Result {
    int code;
    std::string output;
    std::string error;
};

struct Example {
    std::string input;
    std::string output;
    int maximum;
};

class Program {
    fs::path originalDirectory = fs::current_path();
    fs::path directory;

public:
    Program() {
#ifdef _WIN32
        const fs::path executable = originalDirectory / "lab1.exe";
#else
        const fs::path executable = originalDirectory / "lab1";
#endif
        if (!fs::is_regular_file(executable)) {
            throw std::runtime_error("Build lab1 first and run tests from its directory.");
        }

        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        for (int suffix = 0; ; ++suffix) {
            directory = fs::temp_directory_path() /
                ("lab1-tests-" + std::to_string(stamp) + "-" + std::to_string(suffix));
            if (fs::create_directory(directory)) {
                break;
            }
        }
        try {
            fs::copy_file(executable, directory / executable.filename());
            fs::current_path(directory);
        } catch (...) {
            std::error_code error;
            fs::remove_all(directory, error);
            throw;
        }
    }

    ~Program() {
        std::error_code error;
        fs::current_path(originalDirectory, error);
        fs::remove_all(directory, error);
    }

    static void writeFile(const fs::path& path, const std::string& text) {
        std::ofstream file(path, std::ios::binary);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << text;
        file.close();
    }

    static std::string readFile(const fs::path& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot read test file: " + path.string());
        }
        return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    }

    static Result run(const std::string& input) {
        writeFile("stdin.txt", input);
#ifdef _WIN32
        const int code = std::system("lab1.exe < stdin.txt > stdout.txt 2> stderr.txt");
#else
        const int code = std::system("./lab1 < stdin.txt > stdout.txt 2> stderr.txt");
#endif
        return {code, readFile("stdout.txt"), readFile("stderr.txt")};
    }

    static void verify(int mode, const Example& example, bool newline = true) {
        CAPTURE(mode, example.input);
        std::string input = std::to_string(mode) + "\n";
        if (mode == 3) {
            writeFile("input.txt", example.input + (newline ? "\n" : ""));
        } else {
            input += example.input + (newline ? "\n" : "");
        }
        const Result result = run(input);
        REQUIRE(result.code == 0);
        CHECK(result.error.empty());
        CHECK(result.output.find("Результат: [" + example.output + "]") != std::string::npos);
        const bool maximumReported = result.output.find("пробелов: " + std::to_string(example.maximum) + "\r\n") !=
                  std::string::npos ||
              result.output.find("пробелов: " + std::to_string(example.maximum) + "\n") !=
                  std::string::npos;
        CHECK(maximumReported);
        if (mode == 3) {
            CHECK(readFile("output.txt") == example.output);
        }
    }
};

TEST_CASE_METHOD(Program, "All modes process space runs independently", "[processing]") {
    const std::vector<Example> examples = {
        {"", "", 0},
        {"abc", "abc", 0},
        {" ", "", 1},
        {"  ", "", 2},
        {"   ", "*", 3},
        {"    ", "", 4},
        {"     ", "", 5},
        {"      ", "*", 6},
        {"         ", "*", 9},
        {"A   B  C      D", "A*BC*D", 6},
        {"A   B    C", "A*BC", 4},
        {"   abc ", "*abc", 3},
        {" abc      ", "abc*", 6},
        {"A B C", "ABC", 1},
        {"\tA   B\t  C", "\tA*B\tC", 3},
        {"Привет   мир  !", "Привет*мир!", 3},
        {"A*   B**  C", "A**B**C", 3},
        {"[A]   [B]", "[A]*[B]", 3}
    };
    for (int mode = 1; mode <= 3; ++mode) {
        for (const Example& example : examples) {
            verify(mode, example);
        }
    }
}

TEST_CASE_METHOD(Program, "Maximum input length is accepted", "[boundary]") {
    for (int mode = 1; mode <= 3; ++mode) {
        verify(mode, {std::string(4096, 'x'), std::string(4096, 'x'), 0});
        verify(mode, {std::string(4095, ' '), "*", 4095});
        verify(mode, {std::string(4096, ' '), "", 4096});
        verify(mode, {"a" + std::string(4095, ' '), "a*", 4095});
    }
}

TEST_CASE_METHOD(Program, "Overlong input fails without overwriting output", "[boundary][errors]") {
    for (int mode = 1; mode <= 3; ++mode) {
        CAPTURE(mode);
        writeFile("output.txt", "preserve");
        const std::string text(4097, 'x');
        if (mode == 3) {
            writeFile("input.txt", text);
        }
        const Result result = run(std::to_string(mode) + "\n" + (mode == 3 ? "" : text + "\n"));
        CHECK(result.code != 0);
        CHECK_FALSE(result.error.empty());
        CHECK(readFile("output.txt") == "preserve");
    }
}

TEST_CASE_METHOD(Program, "A final newline is optional", "[input]") {
    for (int mode = 1; mode <= 3; ++mode) {
        verify(mode, {"A   B  C", "A*BC", 3}, false);
    }
    verify(3, {"", "", 0}, false);
}

TEST_CASE_METHOD(Program, "File mode reads only the first line", "[files]") {
    writeFile("input.txt", "A   B\nC      D\n");
    const Result result = run("3\n");
    REQUIRE(result.code == 0);
    CHECK(readFile("output.txt") == "A*B");
    CHECK(result.output.find("пробелов: 3") != std::string::npos);
}

TEST_CASE_METHOD(Program, "Missing input file is reported", "[files][errors]") {
    const Result result = run("3\n");
    CHECK(result.code != 0);
    CHECK_FALSE(result.error.empty());
    CHECK_FALSE(fs::exists("output.txt"));
}

TEST_CASE_METHOD(Program, "Output creation failure is reported", "[files][errors]") {
    writeFile("input.txt", "A   B");
    fs::create_directory("output.txt");
    const Result result = run("3\n");
    CHECK(result.code != 0);
    CHECK_FALSE(result.error.empty());
    CHECK(fs::is_directory("output.txt"));
}

TEST_CASE_METHOD(Program, "Invalid menu choices are rejected", "[menu][errors]") {
    for (const std::string& input : std::vector<std::string>{"0\n", "4\n", "12\n", "abc\n", "\n", ""}) {
        CAPTURE(input);
        const Result result = run(input);
        CHECK(result.code != 0);
        CHECK_FALSE(result.error.empty());
    }
}

TEST_CASE_METHOD(Program, "Generated examples match an independent regex reference", "[generated]") {
    std::mt19937 random(20261025);
    const std::regex spaces(" +");
    for (int index = 0; index < 40; ++index) {
        Example example{"", "", 0};
        for (int part = 0; part < 20; ++part) {
            example.input += static_cast<char>('a' + random() % 26);
            example.input.append(random() % 13, ' ');
        }
        std::size_t previous = 0;
        for (std::sregex_iterator match(example.input.begin(), example.input.end(), spaces), end;
             match != end; ++match) {
            const auto position = static_cast<std::size_t>(match->position());
            const auto length = static_cast<int>(match->length());
            example.output += example.input.substr(previous, position - previous);
            if (length % 3 == 0) {
                example.output += '*';
            }
            if (length > example.maximum) {
                example.maximum = length;
            }
            previous = position + static_cast<std::size_t>(length);
        }
        example.output += example.input.substr(previous);
        for (int mode = 1; mode <= 3; ++mode) {
            verify(mode, example);
        }
    }
}
