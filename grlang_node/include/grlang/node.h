#pragma once

#include <cstdint>
#include <memory>
#include <vector>


namespace grlang {
    namespace node {
        struct Node {
            using Ptr = std::shared_ptr<Node>;
            enum class Type : std::uint64_t {
                CONTROL_START,
                CONTROL_STOP,
                CONTROL_RETURN,
                CONTROL_REGION,
                CONTROL_IFELSE,
                CONTROL_PROJECT,

                DATA_TERM,
                DATA_PROJECT,
                DATA_PHI,

                DATA_OP_NEG,
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
            };
            Type type;
            std::vector<Ptr> inputs;
            // std::vector<Ptr::weak_type> outputs;
            union
            {
                int64_t value_int;
            };
        };
    }
}
