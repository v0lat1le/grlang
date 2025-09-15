#include <cassert>
#include <stdexcept>
#include <map>

#include "grlang/node.h"


namespace
{
    using namespace grlang::node;

    const char* get_node_label(Node::Type type) {
        switch (type) {
        case Node::Type::CONTROL_START: return "START";
        case Node::Type::CONTROL_STOP: return "STOP";
        case Node::Type::CONTROL_RETURN: return "RETURN";
        case Node::Type::CONTROL_REGION: return "REGION";
        case Node::Type::CONTROL_IFELSE: return "IFELSE";
        case Node::Type::CONTROL_PROJECT: return "PROJECT";
        case Node::Type::CONTROL_DEAD: return "DEAD";
        case Node::Type::DATA_TERM: return "TERM";
        case Node::Type::DATA_PROJECT: return "PROJECT";
        case Node::Type::DATA_PHI: return "PHI";
        case Node::Type::DATA_CALL: return "CALL";
        case Node::Type::DATA_OP_NEG: return "OP_NEG";
        case Node::Type::DATA_OP_NOT: return "OP_NOT";
        case Node::Type::DATA_OP_BEGIN: return "OP_BEGIN";
        case Node::Type::DATA_OP_ADD: return "OP_ADD";
        case Node::Type::DATA_OP_SUB: return "OP_SUB";
        case Node::Type::DATA_OP_MUL: return "OP_MUL";
        case Node::Type::DATA_OP_DIV: return "OP_DIV";
        case Node::Type::DATA_OP_LT: return "OP_LT";
        case Node::Type::DATA_OP_LEQ: return "OP_LEQ";
        case Node::Type::DATA_OP_GT: return "OP_GT";
        case Node::Type::DATA_OP_GEQ: return "OP_GEQ";
        case Node::Type::DATA_OP_EQ: return "OP_EQ";
        case Node::Type::DATA_OP_NEQ: return "OP_NEQ";
        case Node::Type::DATA_OP_END: return "OP_END";
        default: return "FIXME";
        }
    }

    const char* get_node_shape(Node::Type type) {
        switch (type)
        {
            case Node::Type::CONTROL_START:
            case Node::Type::CONTROL_STOP:
            case Node::Type::CONTROL_RETURN:
            case Node::Type::CONTROL_REGION:
            case Node::Type::CONTROL_IFELSE:
            case Node::Type::CONTROL_PROJECT:
            case Node::Type::CONTROL_DEAD: return "box";

            case Node::Type::DATA_PHI: return "hexagon";
            case Node::Type::DATA_TERM: ;
            case Node::Type::DATA_PROJECT:
            case Node::Type::DATA_CALL:
            case Node::Type::DATA_OP_NEG:
            case Node::Type::DATA_OP_NOT:
            case Node::Type::DATA_OP_BEGIN:
            case Node::Type::DATA_OP_ADD:
            case Node::Type::DATA_OP_SUB:
            case Node::Type::DATA_OP_MUL:
            case Node::Type::DATA_OP_DIV:
            case Node::Type::DATA_OP_LT:
            case Node::Type::DATA_OP_LEQ:
            case Node::Type::DATA_OP_GT:
            case Node::Type::DATA_OP_GEQ:
            case Node::Type::DATA_OP_EQ:
            case Node::Type::DATA_OP_NEQ:
            case Node::Type::DATA_OP_END: return "ellipse";
        default: return "star";
        }
    }

    void print_dot_helper(const Node::Ptr& root, std::ostream& output, std::map<const Node*, std::size_t>& node_ids) {
        assert(!node_ids.contains(root.get()));

        std::size_t root_id = node_ids[root.get()] = node_ids.size();
        output << "  " << root_id << " [label=\""<< get_node_label(root->type) << "\" shape=\""<< get_node_shape(root->type) << "\"]\n";
        for (const Node::Ptr& child: root->inputs) {
            if (!child) {
                continue;
            }
            if (!node_ids.contains(child.get())) {
                print_dot_helper(child, output, node_ids);
            }
            int child_id = node_ids.at(child.get());
            output << "  " << root_id << "->" << child_id << "\n";
        }
    }
}

namespace grlang::node {
    int get_value_int(const Node& node) {
        assert(is_const(node));
        assert(static_cast<const ValueNode&>(node).value.type == Value::Type::INTEGER);
        return static_cast<const ValueNode&>(node).value.integer;
    }

    int (*op_func(grlang::node::Node::Type type))(int, int) {
        switch (type) {
        case grlang::node::Node::Type::DATA_OP_MUL:
            return [](int a, int b)->int { return a*b; };
        case grlang::node::Node::Type::DATA_OP_DIV:
            return [](int a, int b)->int { return a/b; };
        case grlang::node::Node::Type::DATA_OP_ADD:
            return [](int a, int b)->int { return a+b; };
        case grlang::node::Node::Type::DATA_OP_SUB:
            return [](int a, int b)->int { return a-b; };
        case grlang::node::Node::Type::DATA_OP_GT:
            return [](int a, int b)->int { return a>b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_GEQ:
            return [](int a, int b)->int { return a>=b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_LT:
            return [](int a, int b)->int { return a<b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_LEQ:
            return [](int a, int b)->int { return a<=b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_EQ:
            return [](int a, int b)->int { return a==b ? 1 : 0; };
        case grlang::node::Node::Type::DATA_OP_NEQ:
            return [](int a, int b)->int { return a!=b ? 1 : 0; };
        default:
            throw std::runtime_error("bad op");
        }
    }

    void print_dot(const Node::Ptr& root, std::ostream& output) {
        std::map<const Node*, std::size_t> nodes;
        output << "digraph {\n";
        output << "  rankdir=\"BT\"\n";
        print_dot_helper(root, output, nodes);
        output << "}" << std::endl;
    }
}
