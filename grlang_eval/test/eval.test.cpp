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
    assert(grlang::eval::eval(graph, 0) == 0);

    graph = grlang::parse::parse("return -(-2-3)");
    assert(grlang::eval::eval(graph, 0) == 5);

    graph = grlang::parse::parse("return arg");
    assert(grlang::eval::eval(graph, 3) == 3);
    assert(grlang::eval::eval(graph, 7) == 7);

    graph = grlang::parse::parse("a:int=4 return arg-a");
    assert(grlang::eval::eval(graph, 3) == -1);

    graph = grlang::parse::parse("a:int=4 {b:int=3 a=a+b*3} return a-arg");
    assert(grlang::eval::eval(graph, -2) == 15);
}

TEST_CASE(test_branches) {
    auto graph = grlang::parse::parse("if arg<0 arg=-arg return arg");
    assert(grlang::eval::eval(graph, 1) == 1);
    assert(grlang::eval::eval(graph, -1) == 1);

    graph = grlang::parse::parse("if arg<0 if arg<-10 arg=-10 else arg=-1 else if arg>10 arg=10 else arg=1 return arg");
    assert(grlang::eval::eval(graph, -11) == -10);
    assert(grlang::eval::eval(graph, -5) == -1);
    assert(grlang::eval::eval(graph, 11) == 10);
    assert(grlang::eval::eval(graph, 5) == 1);
}

TEST_CASE(test_loops) {
    auto graph = grlang::parse::parse("while arg<10 arg=arg+1 return arg");
    assert(grlang::eval::eval(graph, 9) == 10);
    assert(grlang::eval::eval(graph, 10) == 10);
    assert(grlang::eval::eval(graph, 11) == 11);

    graph = grlang::parse::parse("a:=0 b:=1 i:=0 while i<arg { c:=a+b a=b b=c i=i+1 } return a");
    assert(grlang::eval::eval(graph, 0) == 0);
    assert(grlang::eval::eval(graph, 1) == 1);
    assert(grlang::eval::eval(graph, 5) == 5);
    assert(grlang::eval::eval(graph, 10) == 55);
}

TEST_CASE(test_functions) {
    // auto graph = grlang::parse::parse("f:= (x:int y:int) -> int { return x*y } return f(arg+1 13)");
    // assert(grlang::eval::eval(graph, -1) == 0);
    // assert(grlang::eval::eval(graph, 0) == 13);
    // assert(grlang::eval::eval(graph, 2) == 26);

    auto graph = grlang::parse::parse("f:= (n:int) -> int { if n==0 return 0 if n==1 return 1 return f(n-1)+f(n-2) } return f(arg)");
    assert(grlang::eval::eval(graph, 0) == 0);
    assert(grlang::eval::eval(graph, 1) == 1);
    assert(grlang::eval::eval(graph, 5) == 5);
    assert(grlang::eval::eval(graph, 10) == 55);
}

int main() {
    for (auto [test, name]: registered_tests) {
        std::cout << "running " << name << "..." << std::endl;
        test();
    }
    return 0;
}
