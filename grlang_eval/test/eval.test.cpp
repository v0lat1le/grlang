#include "grtest.h"
#include "grlang/parse.h"
#include "grlang/eval.h"


int run_in_main(std::string code, int arg) {
    std::string main = "main:= (arg:int)->int {\n" + code + "\n}";
    auto exports = grlang::parse::parse_unit(main);
    return grlang::eval::eval_call(exports.at("main"), arg);
}

TEST_CASE(test_sequential) {
    assert(run_in_main("return 0", 0) == 0);
    assert(run_in_main("return -(-2-3)", 0) == 5);
    assert(run_in_main("return arg" , 3) == 3);
    assert(run_in_main("return arg", 7) == 7);
    assert(run_in_main("a:int=4 return arg-a", 3) == -1);
    assert(run_in_main("a:int=4 {b:int=3 a=a+b*3} return a-arg", -2) == 15);
}

TEST_CASE(test_branches) {
    assert(run_in_main("if arg<0 arg=-arg return arg", 1) == 1);
    assert(run_in_main("if arg<0 arg=-arg return arg", -1) == 1);

    assert(run_in_main("if arg<0 if arg<-10 arg=-10 else arg=-1 else if arg>10 arg=10 else arg=1 return arg", -11) == -10);
    assert(run_in_main("if arg<0 if arg<-10 arg=-10 else arg=-1 else if arg>10 arg=10 else arg=1 return arg", -5) == -1);
    assert(run_in_main("if arg<0 if arg<-10 arg=-10 else arg=-1 else if arg>10 arg=10 else arg=1 return arg", 11) == 10);
    assert(run_in_main("if arg<0 if arg<-10 arg=-10 else arg=-1 else if arg>10 arg=10 else arg=1 return arg", 5) == 1);
}

TEST_CASE(test_loops) {
    assert(run_in_main("while arg<10 arg=arg+1 return arg", 9) == 10);
    assert(run_in_main("while arg<10 arg=arg+1 return arg", 10) == 10);
    assert(run_in_main("while arg<10 arg=arg+1 return arg", 11) == 11);

    assert(run_in_main("a:=0 b:=1 i:=0 while i<arg { c:=a+b a=b b=c i=i+1 } return a", 0) == 0);
    assert(run_in_main("a:=0 b:=1 i:=0 while i<arg { c:=a+b a=b b=c i=i+1 } return a", 1) == 1);
    assert(run_in_main("a:=0 b:=1 i:=0 while i<arg { c:=a+b a=b b=c i=i+1 } return a", 5) == 5);
    assert(run_in_main("a:=0 b:=1 i:=0 while i<arg { c:=a+b a=b b=c i=i+1 } return a", 10) == 55);
}

TEST_CASE(test_functions) {
    // auto graph = grlang::parse::parse("f:= (x:int y:int) -> int { return x*y } return f(arg+1 13)");
    // assert(grlang::eval::eval(graph, -1) == 0);
    // assert(grlang::eval::eval(graph, 0) == 13);
    // assert(grlang::eval::eval(graph, 2) == 26);

    auto exports = grlang::parse::parse_unit("fib:= (n:int) -> int { if n==0 return 0 if n==1 return 1 return fib(n-1)+fib(n-2) }");
    auto fib = exports.at("fib");
    assert(grlang::eval::eval_call(fib, 0) == 0);
    assert(grlang::eval::eval_call(fib, 1) == 1);
    assert(grlang::eval::eval_call(fib, 5) == 5);
    assert(grlang::eval::eval_call(fib, 10) == 55);
}
