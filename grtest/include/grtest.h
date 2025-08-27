#include <cassert>
#include <source_location>
#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>


namespace {
    struct TestInfo {
        void(*func)();
        const char* name;
        std::source_location loc;
    };
    std::vector<TestInfo> registered_tests;
    struct test_register {
        test_register(void(*func)(), const char* name, std::source_location loc) {
            registered_tests.emplace_back(func, name, loc);
        }
    };

    int list_tests(std::string target, std::string filename) {
        std::replace(filename.begin(), filename.end(), '\\', '/');
        std::cout << "Generating " << filename << std::endl;
        std::ofstream output(filename);
        for (auto& test: registered_tests) {
            output << "add_test(" << target << "." << test.name << " \"" << target << "\" run-test " << test.name << ")" << std::endl;
            std::string test_file(test.loc.file_name());
            std::replace(test_file.begin(), test_file.end(), '\\', '/');
            output << "set_tests_properties(" << target << "." << test.name << " PROPERTIES _BACKTRACE_TRIPLES \"" << test_file << ";" << test.loc.line() << ";\")" << std::endl;
        }
        return 0;
    }

    int run_test(std::string filter) {
        for (auto test: registered_tests) {
            if (test.name == filter) {
                test.func();
            }
        }
        return 0;
    }

    int run_all_tests() {
        for (auto test: registered_tests) {
            std::cout << "running " << test.name << "..." << std::endl;
            test.func();
        }
        return 0;
    }
}
#define TEST_CASE(name) void name(); test_register test_register_##name(name, #name, std::source_location::current()); void name()


int main(int argc, char* argv[]) {
    if (argc > 3 && argv[1] == std::string_view("list-tests")) {
        return list_tests(argv[2], argv[3]);
    }
    if (argc > 2 && argv[1] == std::string_view("run-test")) {
        return run_test(argv[2]);
    }
    return run_all_tests();
}
