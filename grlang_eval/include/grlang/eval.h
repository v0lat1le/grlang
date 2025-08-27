#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "grlang/node.h"


namespace grlang::eval {
    int eval_call(const node::Node::Ptr& func, int arg);
}
