#include <cassert>

#include "grlang/node.h"
#include "grlang/codegen.h"


namespace {
    const std::string_view MAIN_STUB =
        "define i32 @grlang_main(i32 %arg) {\n"
        "    ret i32 %arg\n"
        "}\n";
}

namespace grlang::codegen {
    bool gen_llvm_ir(const std::unordered_map<std::string_view, node::Node::Ptr>& exports, std::ostream& output) {
        output << MAIN_STUB;
        output.flush();
        return true;
    }
}
