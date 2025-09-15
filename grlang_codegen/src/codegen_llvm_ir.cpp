#include <cassert>
#include <set>
#include <map>

#include "grlang/node.h"
#include "grlang/codegen.h"


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

    using NodeMap = std::map<const grlang::node::Node*, std::vector<const grlang::node::Node*>>;

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

    using Cache = std::map<const grlang::node::Node*, std::size_t>;

    const char* op_code(grlang::node::Node::Type type) {
        switch (type)
        {
        case grlang::node::Node::Type::DATA_OP_MUL:
            return "mul";
        case grlang::node::Node::Type::DATA_OP_DIV:
            return "div";
        case grlang::node::Node::Type::DATA_OP_ADD:
            return "add";
        case grlang::node::Node::Type::DATA_OP_SUB:
            return "sub";
        case grlang::node::Node::Type::DATA_OP_GT:
            return "gt";
        case grlang::node::Node::Type::DATA_OP_GEQ:
            return "ge";
        case grlang::node::Node::Type::DATA_OP_LT:
            return "lt";
        case grlang::node::Node::Type::DATA_OP_LEQ:
            return "le";
        case grlang::node::Node::Type::DATA_OP_EQ:
            return "eq";
        case grlang::node::Node::Type::DATA_OP_NEQ:
            return "neq";
        default:
            throw std::runtime_error("bad op" + std::to_string((int)type));
        }
    }

    int output_expression(const grlang::node::Node::Ptr& node, Cache& cache, std::ostream& output) {
        if (cache.contains(node.get())) {
            return cache.at(node.get());
        }
        if (grlang::node::is_const(*node)) {
            auto expr_id = cache.size();
            output << "    %v" << expr_id << " = add i32 " << grlang::node::get_value_int(*node) << ", 0\n";
            cache[node.get()] = expr_id;
            return expr_id;
        }
        if (is_binary_op(*node)) {
            std::size_t op1 = output_expression(node->inputs.at(0), cache, output);
            std::size_t op2 = output_expression(node->inputs.at(1), cache, output);
            auto expr_id = cache.size();
            output << "    %v" << expr_id << " = " << op_code(node->type) << " i32 %v" << op1 << ", %v" << op2 << "\n";
            cache[node.get()] = expr_id;
            return expr_id;
        }
        switch (node->type) {
            case grlang::node::Node::Type::DATA_TERM:
                throw std::runtime_error("unknown node value");
            case grlang::node::Node::Type::DATA_PROJECT:
                assert(node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_START);
                assert(node->value == 1);  // TODO: support different arities
                return node->value-1;
            // case grlang::node::Node::Type::DATA_OP_NEG:
            //     return -eval_expression(node->inputs.at(0), cache);
            // case grlang::node::Node::Type::DATA_OP_NOT:
            //     return eval_expression(node->inputs.at(0), cache) == 1 ? 0 : 1;
            // case grlang::node::Node::Type::DATA_CALL:
            //     assert(node->inputs.size() == 2);
            //     return grlang::eval::eval_call(node->inputs.at(0), eval_expression(node->inputs.at(1), cache));
            default:
                throw std::runtime_error("unknown node type " +  std::to_string((int)node->type));
        }
    }

    void output_function(std::string_view name, const grlang::node::Node::Ptr& func, std::ostream& output) {
        NodeMap inv_ctl;
        NodeMap reg2phi;
        std::set<const grlang::node::Node*> visited;
        build_node_maps(func->inputs.at(0).get(), inv_ctl, reg2phi, visited);

        const grlang::node::Node::Ptr& start = find_start(func->inputs.at(0));
        
        const std::size_t n_params = 1;  // TODO: figure out number of parames
        std::size_t vars = 0;
        output << "define i32 @" << name << "(";
        while(vars<n_params) {
            output << "i32 %v" << vars++;
            if (vars<n_params) {
                output << ", ";
            }
        }
        output << ") {\n";

        Cache cache;
        cache[start.get()] = cache.size();  // TODO: handle function params properly

        const grlang::node::Node *prev = nullptr;
        const grlang::node::Node *ctl = start.get();
        while (ctl->type != grlang::node::Node::Type::CONTROL_STOP) {
            if (ctl->type == grlang::node::Node::Type::CONTROL_RETURN) {
                std::size_t result = output_expression(ctl->inputs.at(1), cache, output);
                output << "    ret i32 %v" << result << "\n";
                break;
            }
            if (ctl->type == grlang::node::Node::Type::CONTROL_REGION) {
            }
            if (ctl->type == grlang::node::Node::Type::CONTROL_IFELSE) {
                auto cond = output_expression(ctl->inputs.at(1), cache, output);
                output << "    br i1 %v" << cond << " label t" << cond << ", label f" << cond << "\n";
                output << "label t" << cond << ":\n";
                // output true branch till join
                output << "label f" << cond << ":\n";
                // output false branch till join
                // set ctl to the join point
            } else {
                assert(inv_ctl.at(ctl).size() == 1);
                prev = ctl;
                ctl = inv_ctl.at(prev).front();
            }
        }
        output << "}\n";
    }
}

namespace grlang::codegen {
    bool gen_llvm_ir(const std::unordered_map<std::string_view, node::Node::Ptr>& exports, std::ostream& output) {
        for (auto& [name, node]: exports) {
            assert(node->type == node::Node::Type::DATA_TERM);
            if (get_value_int(*node) == 0x0FEFEFE0) {  // TODO function ptr type
                output_function(name, node, output);
            }
        }
        output.flush();
        return true;
    }
}
