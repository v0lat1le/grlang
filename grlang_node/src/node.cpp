#include <cassert>
#include <stdexcept>

#include "grlang/node.h"


namespace grlang::node {
    int get_value_int(const Node& node) {
        assert(is_const(node));
        assert(static_cast<const ValueNode&>(node).value.type == Value::Type::INTEGER);
        return static_cast<const ValueNode&>(node).value.integer;
    }

    int (*op_func(grlang::node::Node::Type type))(int, int) {
        switch (type) {
        case grlang::node::Node::Type::DATA_OP_MUL:
            return [](int a, int b)->int { return a*b; };
        case grlang::node::Node::Type::DATA_OP_DIV:
            return [](int a, int b)->int { return a/b; };
        case grlang::node::Node::Type::DATA_OP_ADD:
            return [](int a, int b)->int { return a+b; };
        case grlang::node::Node::Type::DATA_OP_SUB:
            return [](int a, int b)->int { return a-b; };
        case grlang::node::Node::Type::DATA_OP_GT:
            return [](int a, int b)->int { return a>b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_GEQ:
            return [](int a, int b)->int { return a>=b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_LT:
            return [](int a, int b)->int { return a<b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_LEQ:
            return [](int a, int b)->int { return a<=b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_EQ:
            return [](int a, int b)->int { return a==b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_NEQ:
            return [](int a, int b)->int { return a!=b ? 1 : 0; };
        default:
            throw std::runtime_error("bad op");
        }
    }
}
