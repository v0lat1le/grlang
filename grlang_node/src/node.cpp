#include <cassert>

#include "grlang/node.h"


namespace grlang::node {

    bool is_const(const Node& node) {
        return node.type == Node::Type::DATA_TERM
            && static_cast<const ValueNode&>(node).value.clazz == Value::Class::CONSTANT;
    }

    int get_value_int(const Node& node) {
        assert(is_const(node));
        assert(static_cast<const ValueNode&>(node).value.type == Value::Type::INTEGER);
        return static_cast<const ValueNode&>(node).value.integer;
    }
}
