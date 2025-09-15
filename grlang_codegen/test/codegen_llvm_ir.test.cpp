#include <iostream>
#include <fstream>
#include <iterator>
#include <string>
#include <string_view>

#include "grlang/parse.h"
#include "grlang/codegen.h"


namespace {
    const std::string_view MAIN_SHIM =
        "declare i32 @atoi(i8*)\n"
        "\n"
        "define i32 @main(i32 %argc, i8** %argv) {\n"
        "    %arg_ptr = getelementptr inbounds ptr, ptr %argv, i64 1\n"
        "    %arg_str = load ptr, ptr %arg_ptr\n"
        "    %arg_val = call i32 @atoi(i8* %arg_str)\n"
        "    %result = call i32 @test_main(i32 %arg_val)\n"
        "\n"
        "    %exp_ptr = getelementptr inbounds ptr, ptr %argv, i64 2\n"
        "    %exp_str = load ptr, ptr %exp_ptr\n"
        "    %exp_val = call i32 @atoi(i8* %exp_str)\n"
        "\n"
        "    %cond = icmp ne i32 %result, %exp_val\n"
        "    %ret = zext i1 %cond to i32\n"
        "    ret i32 %ret\n"
        "}\n";

    int codegen(std::unordered_map<std::string_view, grlang::node::Node::Ptr> exports, std::ostream& output) {
        output << MAIN_SHIM << "\n";
        return grlang::codegen::gen_llvm_ir(exports, output) ? 0 : 1;
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage:\n    " << argv[0] << " input.grl [-o output.ll]" << std::endl;
        return 1;
    }
    std::cerr << "Compiling " << argv[1] << "..." << std::endl;
    std::ifstream input(argv[1]);
    std::string code((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    auto exports = grlang::parse::parse_unit(code);
    if (argc > 3 && argv[2] == std::string_view{"-o"}) {
        std::cerr << "Ouputting " << argv[3] << "..." << std::endl;
        std::ofstream output(argv[3]);
        return codegen(std::move(exports), output);
    } else {
        std::ofstream output(argv[3]);
        return codegen(std::move(exports), std::cout);
    }
}
