#include <cassert>

#include "grlang/node.h"


namespace grlang::node {

    bool is_const(const Node::Ptr& node) {
        return node->type == Node::Type::DATA_TERM
            && static_cast<ValueNode&>(*node).value.clazz == Value::Class::CONSTANT;
    }

    int get_value_int(const Node::Ptr& node) {
        assert(is_const(node));
        assert(static_cast<ValueNode&>(*node).value.type ==  Value::Type::INTEGER);
        return static_cast<ValueNode&>(*node).value.integer;
    }
}
