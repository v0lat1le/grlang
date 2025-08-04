#pragma once


#include <string>

#include "grlang/node.h"


namespace grlang::parse {
    std::shared_ptr<grlang::node::Node> parse(std::string_view data);
}
