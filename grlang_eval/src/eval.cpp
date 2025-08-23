#include <cassert>
#include <stdexcept>
#include <map>

#include "grlang/eval.h"


namespace {
    const grlang::node::Node::Ptr& findStart(const grlang::node::Node::Ptr& node) {
        switch (node->type) {
            case grlang::node::Node::Type::CONTROL_START:
                return node;
            case grlang::node::Node::Type::CONTROL_STOP:
            case grlang::node::Node::Type::CONTROL_RETURN:
            case grlang::node::Node::Type::CONTROL_IFELSE:
            case grlang::node::Node::Type::CONTROL_PROJECT:
                return findStart(node->inputs.at(0));
            case grlang::node::Node::Type::CONTROL_REGION:
                return findStart(node->inputs.at(1));
            default:
                throw std::runtime_error("unknown node type");
        }
    }

    using Cache = std::map<const grlang::node::Node*, int>;

    int eval_expression(const grlang::node::Node::Ptr& node, Cache& cache) {
        if (grlang::node::is_const(*node)) {
            return grlang::node::get_value_int(*node);
        }
        if (cache.contains(node.get())) {
            return cache.at(node.get());
        }
        if (is_binary_op(*node)) {
            return op_func(node->type)(
                eval_expression(node->inputs.at(0), cache),
                eval_expression(node->inputs.at(1), cache));
        }
        switch (node->type) {
            case grlang::node::Node::Type::DATA_TERM:
                throw std::runtime_error("unknown node value");
            case grlang::node::Node::Type::DATA_PROJECT:
                assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
                assert(node->value == 1);
                return eval_expression(node->inputs.at(0), cache);
            case grlang::node::Node::Type::DATA_OP_NEG:
                return -eval_expression(node->inputs.at(0), cache);
            case grlang::node::Node::Type::DATA_OP_NOT:
                return eval_expression(node->inputs.at(0), cache) == 1 ? 0 : 1;
            default:
                throw std::runtime_error("unknown node type");
        }
    }
}

namespace grlang::eval {
    int eval(const node::Node::Ptr& graph, int arg) {
        assert(graph->type == node::Node::Type::CONTROL_STOP);
        assert(graph->inputs.size() == 1);
        auto& ret = graph->inputs.at(0);
        assert(ret->type == node::Node::Type::CONTROL_RETURN);

        const node::Node::Ptr& start = findStart(graph);
        Cache cache{{start.get(), arg}};
        return eval_expression(ret->inputs.at(1), cache);
    }
}
