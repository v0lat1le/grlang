#include <cassert>
#include <stdexcept>
#include <map>
#include <set>
#include <queue>
#include <algorithm>
#include <iterator>

#include "grlang/node.h"
#include "grlang/eval.h"


namespace {
    const grlang::node::Node::Ptr& find_start(const grlang::node::Node::Ptr& node) {
        switch (node->type) {
            case grlang::node::Node::Type::CONTROL_START:
                return node;
            case grlang::node::Node::Type::CONTROL_STOP:
            case grlang::node::Node::Type::CONTROL_RETURN:
            case grlang::node::Node::Type::CONTROL_IFELSE:
            case grlang::node::Node::Type::CONTROL_PROJECT:
                return find_start(node->inputs.at(0));
            case grlang::node::Node::Type::CONTROL_REGION:
                return find_start(node->inputs.at(1));
            default:
                throw std::runtime_error("unknown node type");
        }
    }

    using Cache = std::map<const grlang::node::Node*, int>;

    int eval_expression(const grlang::node::Node::Ptr& node, Cache& cache);

    Cache bfs(const grlang::node::Node* graph) {
      std::queue<std::pair<const grlang::node::Node *, int>> queue;
      queue.emplace(graph, 0);
        Cache result;
        while (!queue.empty()) {
            auto [node, depth] = queue.front();
            if (!result.contains(node)) {
                result[node] = depth;
                for (auto& child: node->inputs) {
                    if (child && is_control(*node)) {  // TODO: make filter configurable
                        queue.emplace(child.get(), depth + 1);
                    }
                }
            }
            else {
                assert(result.at(node) <= depth);
            }
            queue.pop();
        }
        return result;
    }

    const grlang::node::Node* find_common_ancestor(const grlang::node::Node::Ptr& node1, const grlang::node::Node::Ptr& node2) {
        assert(node1 != node2);
        Cache bfs1 = bfs(node1.get());
        Cache bfs2 = bfs(node2.get());
        std::vector<std::pair<const grlang::node::Node*, int>> common;
        std::set_intersection(bfs1.begin(), bfs1.end(), bfs2.begin(), bfs2.end(), std::back_inserter(common), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
        auto min_common = std::min_element(common.begin(), common.end(), [&bfs1](const auto& lhs, const auto& rhs) { return bfs1.at(lhs.first) < bfs1.at(rhs.first); });
        return min_common->first;
    }

    int eval_phi(const grlang::node::Node::Ptr& node, Cache& cache) {
        assert(node->type == grlang::node::Node::Type::DATA_PHI);
        auto ifelse = find_common_ancestor(node->inputs.at(0)->inputs.at(1), node->inputs.at(0)->inputs.at(2));
        assert(ifelse->type == grlang::node::Node::Type::CONTROL_IFELSE);

        if (eval_expression(ifelse->inputs.at(1), cache) == 1) {
            return eval_expression(node->inputs.at(1), cache);
        } else {
            return eval_expression(node->inputs.at(2), cache);
        }
    }

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
            case grlang::node::Node::Type::DATA_PHI:
                return eval_phi(node, cache);
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

        const node::Node::Ptr& start = find_start(graph);
        Cache cache{{start.get(), arg}};
        return eval_expression(ret->inputs.at(1), cache);
    }
}
