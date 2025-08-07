#pragma once


#include <string>

#include "grlang/node.h"


namespace grlang::parse {
    grlang::node::Node::Ptr parse(std::string_view data);
}
