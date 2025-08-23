#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "grlang/node.h"


namespace grlang::eval {
    int eval(const node::Node::Ptr& graph, int arg);
}
