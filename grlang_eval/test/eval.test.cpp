#include <cassert>
#include <iostream>

#include "grlang/parse.h"
#include "grlang/eval.h"


static std::vector<std::pair<void(*)(), const char*>> registered_tests;
struct test_register {
    test_register(void(*test)(), const char* name) {
        registered_tests.emplace_back(test, name);
    }
};
#define TEST_CASE(name) void name(); test_register test_register_##name(name, #name); void name()


TEST_CASE(test_sequential) {
    auto graph = grlang::parse::parse("return 0");
    auto result = grlang::eval::eval(graph, 0);
    assert(result == 0);

    graph = grlang::parse::parse("return -(-2-3)");
    result = grlang::eval::eval(graph, 0);
    assert(result == 5);

    graph = grlang::parse::parse("return arg");
    result = grlang::eval::eval(graph, 3);
    assert(result == 3);
    result = grlang::eval::eval(graph, 7);
    assert(result == 7);

    graph = grlang::parse::parse("a:int=4 return arg-a");
    result = grlang::eval::eval(graph, 3);
    assert(result == -1);

    graph = grlang::parse::parse("a:int=4 {b:int=3 a=a+b*3} return a-arg");
    result = grlang::eval::eval(graph, -2);
    assert(result == 15);
}

TEST_CASE(test_branches) {
    auto graph = grlang::parse::parse("if arg<0 arg=-arg return arg");
    auto result = grlang::eval::eval(graph, 1);
    assert(result == 1);
    result = grlang::eval::eval(graph, -1);
    assert(result == 1);

    graph = grlang::parse::parse("if arg<0 if arg<-10 arg=-10 else arg=-1 else if arg>10 arg=10 else arg=1 return arg");
    assert(grlang::eval::eval(graph, -11) == -10);
    assert(grlang::eval::eval(graph, -5) == -1);
    assert(grlang::eval::eval(graph, 11) == 10);
    assert(grlang::eval::eval(graph, 5) == 1);
}

TEST_CASE(test_loops) {
}

int main() {
    for (auto [test, name]: registered_tests) {
        std::cout << "running " << name << "..." << std::endl;
        test();
    }
    return 0;
}
