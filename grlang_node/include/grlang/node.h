#pragma once

#include <cstdint>
#include <memory>
#include <vector>


namespace grlang {
    namespace node {
        enum class NodeType : std::uint64_t {
            CONTROL_START,
            CONTROL_STOP,
            CONTROL_REGION,
            CONTROL_RETURN,
            CONTROL_IFELSE,

            CONSTANT_INT,
            DATA_PHI,
            DATA_VAR,

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
