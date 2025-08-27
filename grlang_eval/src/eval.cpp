#include <cassert>
#include <stdexcept>
#include <map>
#include <set>
#include <algorithm>

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
    using NodeMap = std::map<const grlang::node::Node*, std::vector<const grlang::node::Node*>>;

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
                assert(node->value == 1);  // TODO: support different arities
                return eval_expression(node->inputs.at(0), cache);
            case grlang::node::Node::Type::DATA_OP_NEG:
                return -eval_expression(node->inputs.at(0), cache);
            case grlang::node::Node::Type::DATA_OP_NOT:
                return eval_expression(node->inputs.at(0), cache) == 1 ? 0 : 1;
            case grlang::node::Node::Type::DATA_CALL:
                assert(node->inputs.size() == 2);
                return grlang::eval::eval_call(node->inputs.at(0), eval_expression(node->inputs.at(1), cache));
            default:
                throw std::runtime_error("unknown node type");
        }
    }

    void build_node_maps(const grlang::node::Node *node, NodeMap& inv_ctl, NodeMap& reg2phi, std::set<const grlang::node::Node*>& visited) {
        if (visited.contains(node)) {
            return;
        }
        visited.insert(node);
        switch (node->type) {
            case grlang::node::Node::Type::DATA_PHI:
                reg2phi[node->inputs.at(0).get()].push_back(node);
                break;
            case grlang::node::Node::Type::CONTROL_STOP:
            case grlang::node::Node::Type::CONTROL_RETURN:
            case grlang::node::Node::Type::CONTROL_IFELSE:
            case grlang::node::Node::Type::CONTROL_PROJECT:
                inv_ctl[node->inputs.at(0).get()].push_back(node);
                break;
            case grlang::node::Node::Type::CONTROL_REGION:
                inv_ctl[node->inputs.at(1).get()].push_back(node);
                inv_ctl[node->inputs.at(2).get()].push_back(node);
                break;
            default:
                break;
        }
        for (auto &child : node->inputs) {
            if (child) {
                build_node_maps(child.get(), inv_ctl, reg2phi, visited);
            }
        }
    }

    int eval_graph(const grlang::node::Node *ctl, Cache& cache, NodeMap &inv_ctl, NodeMap &reg2phi) {
        assert(ctl->type == grlang::node::Node::Type::CONTROL_START);
        const grlang::node::Node* prev = nullptr;
        while (true) {
            if (ctl->type == grlang::node::Node::Type::CONTROL_RETURN) {
                return eval_expression(ctl->inputs.at(1), cache);
            }
            if (ctl->type == grlang::node::Node::Type::CONTROL_REGION) {
                auto prev_idx = std::distance(ctl->inputs.begin(),
                    std::find_if(ctl->inputs.begin(), ctl->inputs.end(), [&prev](auto& node) { return node.get() == prev; }));
                Cache tmp;
                for (auto& phi: reg2phi.at(ctl)) {
                    tmp[phi] = eval_expression(phi->inputs.at(prev_idx), cache);
                }
                tmp.merge(cache);
                cache.swap(tmp);
            }
            if (ctl->type == grlang::node::Node::Type::CONTROL_IFELSE) {
                const grlang::node::Node* next_ctl;
                if (eval_expression(ctl->inputs.at(1), cache) == 1) {
                    next_ctl = inv_ctl.at(ctl).at(0)->value==0 ? inv_ctl.at(ctl).at(0) : inv_ctl.at(ctl).at(1);
                } else {
                    next_ctl = inv_ctl.at(ctl).at(0)->value==0 ? inv_ctl.at(ctl).at(1) : inv_ctl.at(ctl).at(0);
                }
                prev = ctl;
                ctl = next_ctl;
            } else {
                assert(inv_ctl.at(ctl).size() == 1);
                prev = ctl;
                ctl = inv_ctl.at(prev).front();
            }
        }
    }
}

namespace grlang::eval {
    int eval_call(const node::Node::Ptr& func, int arg) {
        assert(func->type == node::Node::Type::DATA_TERM);  // TODO: func ptr type
        assert(func->inputs.at(0)->type == node::Node::Type::CONTROL_STOP);
        const node::Node::Ptr& start = find_start(func->inputs.at(0));
        Cache cache{{start.get(), arg}};
        NodeMap inv_ctl;
        NodeMap reg2phi;
        std::set<const grlang::node::Node*> visited;
        build_node_maps(func->inputs.at(0).get(), inv_ctl, reg2phi, visited);
        return eval_graph(start.get(), cache, inv_ctl, reg2phi);
    }
}
