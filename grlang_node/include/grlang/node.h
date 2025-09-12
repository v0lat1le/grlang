#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <ostream>


namespace grlang::node {
    struct Node {
        using Ptr = std::shared_ptr<Node>;
        enum class Type : std::uint8_t {
            CONTROL_START,
            CONTROL_STOP,
            CONTROL_RETURN,
            CONTROL_REGION,
            CONTROL_IFELSE,
            CONTROL_PROJECT,
            CONTROL_DEAD,

            DATA_TERM,
            DATA_PROJECT,
            DATA_PHI,
            DATA_CALL,

            DATA_OP_NEG,
            DATA_OP_NOT,
            DATA_OP_BEGIN,  // NOTE: put binary operation tags between DATA_OP_BEGIN and DATA_OP_END
            DATA_OP_ADD,
            DATA_OP_SUB,
            DATA_OP_MUL,
            DATA_OP_DIV,
            DATA_OP_LT,
            DATA_OP_LEQ,
            DATA_OP_GT,
            DATA_OP_GEQ,
            DATA_OP_EQ,
            DATA_OP_NEQ,
            DATA_OP_END,
        };
        Type type;
        uint8_t value;
        uint16_t depth;
        std::vector<Ptr> inputs;
        // std::vector<Ptr::weak_type> outputs;

        Node(Type type_, uint8_t value_, std::initializer_list<Ptr> inputs_) : type(type_), value(value_), depth(0), inputs(inputs_) {}
    };

    struct Value {
        enum class Class : std::uint8_t {
            TOP_TYPE,
            TOP_CONST,
            CONSTANT,
            VARIABLE,
            BOT_TYPE,
        };
        enum class Type : std::uint16_t {
            UNKNOWN,
            INTEGER,
            TUPLE,
        };
        Class clazz = Class::TOP_TYPE;
        Type type = Type::UNKNOWN;
        union
        {
            int integer;
            //std::vector<Value> tuple;
        };

        Value() : clazz(Class::TOP_TYPE), type(Type::UNKNOWN), integer(0) {}
        Value(int v) : clazz(Class::CONSTANT), type(Type::INTEGER), integer(v) {}
        Value(Type t) : clazz(Class::VARIABLE), type(t), integer(0) {}
        ~Value() {
            if (type == Type::TUPLE) {
                //tuple.~vector();
            }
        }
    };

    struct ValueNode : Node {
        Value value;
    };

    inline bool is_binary_op(const Node& node) {
        return node.type > Node::Type::DATA_OP_BEGIN && node.type <Node::Type::DATA_OP_END;
    }
    inline bool is_control(const Node& node) {
        return node.type <= Node::Type::CONTROL_DEAD;
    }
    inline bool is_data(const Node& node) {
        return !is_control(node);
    }

    inline bool is_const(const Node& node) {
        return node.type == Node::Type::DATA_TERM && static_cast<const ValueNode&>(node).value.clazz == Value::Class::CONSTANT;
    }

    int get_value_int(const Node& node);

    int (*op_func(grlang::node::Node::Type type))(int, int);

    void print_dot(const node::Node::Ptr& root, std::ostream& output);
}
