#include <cassert>
#include <iostream>
#include <vector>

#include "grlang/compile.h"


static std::vector<std::pair<void(*)(), const char*>> registered_tests;
struct test_register {
    test_register(void(*test)(), const char* name) {
        registered_tests.emplace_back(test, name);
    }
};
#define TEST_CASE(name) void name(); test_register test_register_##name(name, #name); void name()

int main() {
    for (auto [test, name]: registered_tests) {
        std::cout << "running " << name << "..." << std::endl;
        test();
    }
    return 0;
}
