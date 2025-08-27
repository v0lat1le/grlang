#pragma once


#include <string>
#include <unordered_map>
#include "grlang/node.h"


namespace grlang::parse {
    std::unordered_map<std::string_view, grlang::node::Node::Ptr> parse_unit(std::string_view code);
}
