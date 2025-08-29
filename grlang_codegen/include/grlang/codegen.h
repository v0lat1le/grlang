#pragma once

#include <string>
#include <unordered_map>
#include <ostream>

#include "grlang/node.h"

namespace grlang::codegen {
    bool gen_llvm_ir(const std::unordered_map<std::string_view, node::Node::Ptr>& exports, std::ostream& output);
}
