#include <stdexcept>
#include <cstdint>
#include <cassert>
#include <unordered_set>
#include <unordered_map>

#include "grlang/detail/token.h"
#include "grlang/parse.h"


namespace {
    using grlang::parse::detail::TokenType;

    int svtoi(const std::string_view& sv) {
        int result = 0;
        for (char c: sv) {
            result = result*10 + c - '0';
        }
        return result;
    }

    grlang::node::Node::Type operation_type(grlang::parse::detail::TokenType token) {
        switch (token) {
            case TokenType::OPERATOR_PLUS:
                return grlang::node::Node::Type::DATA_OP_ADD;
            case TokenType::OPERATOR_MINUS:
                return grlang::node::Node::Type::DATA_OP_SUB;
            case TokenType::OPERATOR_STAR:
                return grlang::node::Node::Type::DATA_OP_MUL;
            case TokenType::OPERATOR_SLASH:
                return grlang::node::Node::Type::DATA_OP_DIV;
            case TokenType::OPERATOR_LT:
                return grlang::node::Node::Type::DATA_OP_LT;
            case TokenType::OPERATOR_GT:
                return grlang::node::Node::Type::DATA_OP_GT;
            case TokenType::OPERATOR_LEQ:
                return grlang::node::Node::Type::DATA_OP_LEQ;
            case TokenType::OPERATOR_GEQ:
                return grlang::node::Node::Type::DATA_OP_GEQ;
            case TokenType::OPERATOR_EQ:
                return grlang::node::Node::Type::DATA_OP_EQ;
            case TokenType::OPERATOR_NEQ:
                return grlang::node::Node::Type::DATA_OP_NEQ;
            default:
                throw std::runtime_error("Unsupported token type");
        }
    }

    std::uint8_t operation_precedence(grlang::node::Node::Type type) {
        switch (type) {
            case grlang::node::Node::Type::DATA_OP_NEG:
            case grlang::node::Node::Type::DATA_OP_NOT:
                return 2;
            case grlang::node::Node::Type::DATA_OP_MUL:
            case grlang::node::Node::Type::DATA_OP_DIV:
                return 3;
            case grlang::node::Node::Type::DATA_OP_ADD:
            case grlang::node::Node::Type::DATA_OP_SUB:
                return 4;
            case grlang::node::Node::Type::DATA_OP_GT:
            case grlang::node::Node::Type::DATA_OP_GEQ:
            case grlang::node::Node::Type::DATA_OP_LT:
            case grlang::node::Node::Type::DATA_OP_LEQ:
                return 6;
            case grlang::node::Node::Type::DATA_OP_EQ:
            case grlang::node::Node::Type::DATA_OP_NEQ:
                return 7;
            default:
                return 255;
        }
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type, std::uint8_t value, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        return std::make_shared<grlang::node::Node>(type, value, inputs);
    };

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        return std::make_shared<grlang::node::Node>(type, std::uint8_t(0), inputs);
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type) {
        return make_node(type, 0, {});
    }

    grlang::node::Node::Ptr make_value_node(int value) {
        // TODO: cache constants like 0 and 1
        return std::make_shared<grlang::node::ValueNode>(grlang::node::Node(grlang::node::Node::Type::DATA_TERM, 0, {}), grlang::node::Value(value));
    }

    grlang::node::Node::Ptr peephole(grlang::node::Node::Ptr node) {
        if (grlang::node::is_binary_op(*node) && is_const(*node->inputs.at(0)) && is_const(*node->inputs.at(1))) {
            return make_value_node(op_func(node->type)(get_value_int(*node->inputs.at(0)), get_value_int(*node->inputs.at(1))));
        }
        switch (node->type) {
            case grlang::node::Node::Type::DATA_OP_NEG:
                if (is_const(*node->inputs.at(0))) {
                    return make_value_node(-get_value_int(*node->inputs.at(0)));
                }
                break;
            case grlang::node::Node::Type::DATA_OP_NOT:
                if (is_const(*node->inputs.at(0))) {
                    return make_value_node(get_value_int(*node->inputs.at(0)) == 0 ? 1 : 0);
                }
                break;
            case grlang::node::Node::Type::DATA_OP_ADD:
                if (is_const(*node->inputs.at(0))) {
                    std::swap(node->inputs.at(0), node->inputs.at(1));
                }
                if (is_const(*node->inputs.at(1))) {
                    if (get_value_int(*node->inputs.at(1))==0) {
                        return node->inputs.at(0);
                    }
                    if (node->inputs.at(0)->type == grlang::node::Node::Type::DATA_OP_ADD && is_const(*node->inputs.at(0)->inputs.at(1))) {
                        return make_node(grlang::node::Node::Type::DATA_OP_ADD, {node->inputs.at(0)->inputs.at(0), make_value_node(get_value_int(*node->inputs.at(0)->inputs.at(1)) + get_value_int(*node->inputs.at(1)))});
                    }
                }
                break;
            case grlang::node::Node::Type::DATA_PHI:
                if (node->inputs.at(0)->inputs.at(1)->type == grlang::node::Node::Type::CONTROL_DEAD) {
                    return node->inputs.at(2);
                }
                if (node->inputs.at(0)->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_DEAD) {
                    return node->inputs.at(1);
                }
                break;
            case grlang::node::Node::Type::CONTROL_PROJECT:
                if (node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_IFELSE && is_const(*node->inputs.at(0)->inputs.at(1))) {
                    if ((node->value==0) == (get_value_int(*node->inputs.at(0)->inputs.at(1))==1)) {
                        return node->inputs.at(0)->inputs.at(0);
                    } else {
                        return make_node(grlang::node::Node::Type::CONTROL_DEAD);
                    }
                }
                break;
            case grlang::node::Node::Type::CONTROL_REGION:
                if (node->inputs.at(1)->type == grlang::node::Node::Type::CONTROL_DEAD) {
                    return node->inputs.at(2);
                }
                if (node->inputs.at(2)->type == grlang::node::Node::Type::CONTROL_DEAD) {
                    return node->inputs.at(1);
                }
                break;
            default:
                return node;
        }
        return node;
    }

    grlang::node::Node::Ptr make_peep_node(grlang::node::Node::Type type, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        return peephole(make_node(type, inputs));
    }

    grlang::node::Node::Ptr make_peep_node(grlang::node::Node::Type type, std::uint8_t value, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        return peephole(make_node(type, value, inputs));
    }

    struct Scope {
        using Dict = std::unordered_map<std::string_view, grlang::node::Node::Ptr>;

        std::vector<Dict> stack;
        grlang::node::Node::Ptr control;

        grlang::node::Node::Ptr lookup(const std::string_view& name) const {
            for (auto dict = stack.rbegin(); dict!=stack.rend(); ++dict) {
                if (auto it = dict->find(name); it != dict->end()) {
                    return it->second;
                }
            }
            return nullptr;
        }

        void update(const std::string_view& name, grlang::node::Node::Ptr node) {
            for (auto dict = stack.rbegin(); dict!=stack.rend(); ++dict) {
                if (auto it = dict->find(name); it != dict->end()) {
                    it->second = node;
                    return;
                }
            }
            throw std::runtime_error("not defined");
        }

        void declare(const std::string_view& name, grlang::node::Node::Ptr node) {
            for (auto dict = stack.rbegin(); dict!=stack.rend(); ++dict) {
                if (auto it = dict->find(name); it != dict->end()) {
                    throw std::runtime_error("already defined");
                }
            }
            stack.back()[name] = node;
        }

        grlang::node::Node::Ptr merge(const Scope& src1, const Scope& src2) {
            auto region = make_node(grlang::node::Node::Type::CONTROL_REGION, {nullptr, src1.control, src2.control});
            for (std::size_t i=0; i<stack.size(); ++i) {
                for (auto& [key, val] : stack.at(i)) {
                    if (src1.stack.at(i).at(key) != src2.stack.at(i).at(key)) {  // TODO: make into a value comparison
                        val = make_peep_node(grlang::node::Node::Type::DATA_PHI, {region, src1.stack.at(i).at(key), src2.stack.at(i).at(key)});
                    } else {
                        val = src1.stack.at(i).at(key);
                    }
                }
            }
            return control = peephole(region);
        }

        grlang::node::Node::Ptr start_loop() {
            auto region = make_node(grlang::node::Node::Type::CONTROL_REGION, {nullptr, control, nullptr});
            for (std::size_t i=0; i<stack.size(); ++i) {
                for (auto& [key, val] : stack.at(i)) {
                    val = make_node(grlang::node::Node::Type::DATA_PHI, {region, val, nullptr});
                }
            }
            return region;
        }

        void end_loop(const Scope& loop_scope) {
            for (std::size_t i=0; i<stack.size(); ++i) {
                for (auto& [key, val] : stack.at(i)) {
                    assert(val->type == grlang::node::Node::Type::DATA_PHI);
                    assert(val->inputs.at(2) == nullptr);
                    if (val != loop_scope.stack.at(i).at(key)) {
                        val->inputs.at(2) = loop_scope.stack.at(i).at(key);
                    } else {
                        val->inputs.at(2) = val->inputs.at(1);
                    }
                }
            }
        }
    };

    struct LoopState {
        Scope* break_;
        Scope* continue_;
    };

    struct Parser {
        std::string_view code;
        grlang::parse::detail::Token next_token;

        Parser(std::string_view code_) : code(code_) {
            read_next_token();
        }

        void read_next_token() {
            next_token = grlang::parse::detail::read_token(code);
        }
    };

    grlang::node::Node::Ptr parse_expression(Parser& parser, const Scope& scope, std::uint8_t prev_precedence) {
        grlang::node::Node::Ptr result;
        switch (parser.next_token.type) {
            case TokenType::OPERATOR_MINUS: {
                parser.read_next_token();
                auto precedence = operation_precedence(grlang::node::Node::Type::DATA_OP_NEG);
                result = make_peep_node(grlang::node::Node::Type::DATA_OP_NEG, {parse_expression(parser, scope, precedence)});
                break;
            }
            case TokenType::OPERATOR_NOT: {
                parser.read_next_token();
                auto precedence = operation_precedence(grlang::node::Node::Type::DATA_OP_NOT);
                result = make_peep_node(grlang::node::Node::Type::DATA_OP_NOT, {parse_expression(parser, scope, precedence)});
                break;
            }
            case TokenType::OPEN_ROUND:
                parser.read_next_token();
                result = parse_expression(parser, scope, 255);
                if (parser.next_token.type != TokenType::CLOSE_ROUND) {
                    throw std::runtime_error("Expected )");
                }
                parser.read_next_token();
                break;
            
            case TokenType::IDENTIFIER:
                result = scope.lookup(parser.next_token.value);
                parser.read_next_token();
                break;
            case TokenType::LITERAL_INT:
                result = make_value_node(svtoi(parser.next_token.value));
                parser.read_next_token();
                break;
            default:
                throw std::runtime_error("Expected operand!");
        }

        while (parser.next_token.type > TokenType::META_BINARY_BEGIN && parser.next_token.type < TokenType::META_BINARY_END) {
            auto node_type = operation_type(parser.next_token.type);
            auto precedence = operation_precedence(node_type);
            if (prev_precedence < precedence) {
                break;
            }
            parser.read_next_token();
            result = make_peep_node(node_type, {result, parse_expression(parser, scope, precedence)});
        }

        return result;
    }

    void parse_statement(Parser &parser, Scope &scope, const LoopState &loop, const grlang::node::Node::Ptr &stop);

    void parse_ifelse(Parser& parser, Scope& scope, const LoopState& loop, const grlang::node::Node::Ptr& stop) {
        assert(parser.next_token.value == "if");
        parser.read_next_token();
        auto condition = parse_expression(parser, scope, 255);
        auto ifelse = make_peep_node(grlang::node::Node::Type::CONTROL_IFELSE, {scope.control, condition});
        
        auto true_scope = scope;
        true_scope.control = make_peep_node(grlang::node::Node::Type::CONTROL_PROJECT, 0, {ifelse});
        parse_statement(parser, true_scope, loop, stop);
        
        auto false_scope = scope;
        false_scope.control = make_peep_node(grlang::node::Node::Type::CONTROL_PROJECT, 1, {ifelse});
        if (parser.next_token.value == "else")
        {
            parser.read_next_token();
            parse_statement(parser, false_scope, loop, stop);
        }
        scope.merge(true_scope, false_scope);
    }

    void parse_while(Parser& parser, Scope& scope, const grlang::node::Node::Ptr& stop) {
        assert(parser.next_token.value == "while");
        parser.read_next_token();
        auto loop_region = scope.start_loop();
        auto condition = parse_expression(parser, scope, 255);
        auto ifelse = make_node(grlang::node::Node::Type::CONTROL_IFELSE, {loop_region, condition});
        scope.control = make_node(grlang::node::Node::Type::CONTROL_PROJECT, 1, {ifelse});
        auto loop_scope = scope;
        loop_scope.control = make_node(grlang::node::Node::Type::CONTROL_PROJECT, 0, {ifelse});

        auto base_scope = scope;
        Scope continue_scope{};
        continue_scope.control = loop_scope.control;
        parse_statement(parser, loop_scope, {&scope, &continue_scope}, stop);
        if (continue_scope.stack.empty()) {
            continue_scope = loop_scope;
        } else {
            continue_scope.merge(continue_scope, loop_scope);
        }
        loop_region->inputs.at(2) = continue_scope.control;
        base_scope.end_loop(continue_scope);
    }

    void parse_break(Parser& parser, Scope& scope, const LoopState& loop) {
        assert(parser.next_token.value == "break");
        parser.read_next_token();
        if (loop.break_ == nullptr) {
           throw std::runtime_error("break outside of a loop!");
        }
        loop.break_->merge(scope, *loop.break_);
    }

    void parse_continue(Parser& parser, Scope& scope, const LoopState& loop) {
        assert(parser.next_token.value == "continue");
        parser.read_next_token();
        if (loop.continue_ == nullptr) {
           throw std::runtime_error("continue outside of a loop!");
        }
        if (loop.continue_->stack.empty()) {
            *loop.continue_ = scope;
        } else {
            loop.continue_->merge(*loop.continue_, scope);
        }
    }

    void parse_block(Parser& parser, Scope& scope, const LoopState& loop, const grlang::node::Node::Ptr& stop) {
        while (parser.next_token.type != TokenType::CLOSE_CURLY && parser.next_token.type != TokenType::END_OF_INPUT) {
            parse_statement(parser, scope, loop, stop);
        }
    }

    void parse_type(Parser& parser);

    void parse_struct(Parser& parser) {
        assert(parser.next_token.type == TokenType::OPEN_CURLY);
        do {
            parser.read_next_token();
            if (parser.next_token.type != TokenType::IDENTIFIER) {
                throw std::runtime_error("expected identifier");
            }
            // auto name = parser.next_token.value;  // TODO: check is not a keyword
            parser.read_next_token();
            if (parser.next_token.type != TokenType::DECLARE_TYPE) {
                throw std::runtime_error("expected :");
            }
            parser.read_next_token();
            parse_type(parser);
        } while (parser.next_token.type == TokenType::COMMA);
        if (parser.next_token.type != TokenType::CLOSE_CURLY) {
            throw std::runtime_error("expected }");
        }
        parser.read_next_token();
    }

    void parse_type(Parser& parser)
    {
        if (parser.next_token.type != TokenType::OPEN_CURLY) {
            static std::unordered_set<std::string_view> builtin_types = {"int", "struct", "funct"};
            assert(builtin_types.contains(parser.next_token.value));
            parser.read_next_token();
            return;
        }
        parse_struct(parser);
        if (parser.next_token.type == TokenType::ARROW) {
            parser.read_next_token();
            parse_type(parser);
        }
    }

    void parse_identifier_things(Parser& parser, Scope& scope) {
        assert(parser.next_token.type == TokenType::IDENTIFIER);
        auto name = parser.next_token.value;  // TODO: check is not a keyword
        parser.read_next_token();
        auto scope_update = &Scope::update;
        switch (parser.next_token.type) {
            case TokenType::DECLARE_AUTO:
                parser.read_next_token();
                scope_update = &Scope::declare;
                break;
            case TokenType::DECLARE_TYPE:
                parser.read_next_token();
                parse_type(parser);  // TODO: handle different types
                scope_update = &Scope::declare;
                if (parser.next_token.type != TokenType::REBIND) {
                    throw std::runtime_error("Expected assignment");
                }
                parser.read_next_token();
                break;
            case TokenType::REBIND:
                parser.read_next_token();
                break;
            default:
                throw std::runtime_error("Expected declaration or assignment");
        }
        (scope.*scope_update)(name, parse_expression(parser, scope, 255));
    }

    void parse_statement(Parser& parser, Scope& scope, const LoopState& loop, const grlang::node::Node::Ptr& stop) {
        switch (parser.next_token.type) {
        case TokenType::END_OF_INPUT:
            break;
        case TokenType::OPEN_CURLY: {
            parser.read_next_token();
            scope.stack.emplace_back();
            parse_block(parser, scope, loop, stop);
            if (parser.next_token.type != TokenType::CLOSE_CURLY) {
                throw std::runtime_error("Expected }");
            }
            scope.stack.pop_back();
            parser.read_next_token();
            break;
        }
        case TokenType::IDENTIFIER: {
            if (parser.next_token.value == "return") {
                parser.read_next_token();
                auto result = make_peep_node(grlang::node::Node::Type::CONTROL_RETURN, {scope.control, parse_expression(parser, scope, 255)});
                stop->inputs.push_back(result);
            } else if (parser.next_token.value == "if") {
                parse_ifelse(parser, scope, loop, stop);
            } else if (parser.next_token.value == "while") {
                parse_while(parser, scope, stop);
            } else if (parser.next_token.value == "break") {
                parse_break(parser, scope, loop);
            } else if (parser.next_token.value == "continue") {
                parse_continue(parser, scope, loop);
            } else {
                parse_identifier_things(parser, scope);
            }
            break;
        }
        default:
            throw std::runtime_error("Unexpected token");
        }
    }
}

grlang::node::Node::Ptr grlang::parse::parse(std::string_view code) {
    Parser parser(code);
    Scope scope;
    scope.stack.emplace_back();
    scope.control = make_node(grlang::node::Node::Type::CONTROL_START);
    scope.declare("arg", make_node(grlang::node::Node::Type::DATA_PROJECT, 1, {scope.control}));
    auto stop = make_node(grlang::node::Node::Type::CONTROL_STOP);
    parse_block(parser, scope, {}, stop);
    return stop;
}
