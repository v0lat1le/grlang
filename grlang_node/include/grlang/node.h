#pragma once

#include <memory>
#include <vector>


namespace grlang {
    namespace node {
        enum class NodeType {
            START,
            RETURN,
            CONSTANT_INT,
            OPERATION_ADD,
            OPERATION_MUL,
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
