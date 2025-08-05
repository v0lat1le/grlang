#pragma once

#include <memory>
#include <vector>


namespace grlang {
    namespace node {
        enum class NodeType {
            START,
            RETURN,
            CONSTANT_INT,
            OPERATION_NEG,
            OPERATION_ADD,
            OPERATION_SUB,
            OPERATION_MUL,
            OPERATION_DIV,
            OPERATION_LT,
            OPERATION_LEQ,
            OPERATION_GT,
            OPERATION_GEQ,
            OPERATION_EQ,
            OPERATION_NEQ,
        };

        struct Node {
            NodeType type;
            std::vector<std::shared_ptr<Node>> inputs;
            std::vector<std::weak_ptr<Node>> outputs;
            union
            {
                int64_t value_int;
            };
        };
    }
}
