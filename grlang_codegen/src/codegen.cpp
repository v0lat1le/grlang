#include <cassert>

#include "grlang/node.h"
#include "grlang/codegen.h"


namespace grlang::compile {
    bool gen_llvm_ir(const std::unordered_map<std::string_view, node::Node::Ptr>& exports, std::ostream& output) {
        return true;
    }
}
