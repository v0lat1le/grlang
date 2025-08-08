#include <stdexcept>
#include <cstdint>
#include <cassert>
#include <unordered_map>

#include "grlang/parse.h"


namespace {
    std::string_view::size_type count_class(const std::string_view& code, int (*isclass)(int)) {
        std::string_view::size_type n=0;
        while(n<code.size() && isclass(code[n])) {
            n = n+1;
        }
        return n;
    }

    std::string_view read_chars(std::string_view& code, std::size_t n) {
        auto result = code.substr(0, n);
        code.remove_prefix(n);
        return result;
    }

    std::string_view read_class(std::string_view& code, int (*isclass)(int)) {
        auto n = count_class(code, isclass);
        return read_chars(code, n);
    }

    void skip_whitespace(std::string_view& code) {
        code.remove_prefix(count_class(code, isspace));
    }

    std::string_view read_number(std::string_view& code) {
        return read_class(code, std::isdigit);
    }

    std::string_view read_identifier(std::string_view& code) {
        return read_class(code, std::isalnum);
    }

    int svtoi(const std::string_view& sv) {
        int result = 0;
        for (char c: sv) {
            result = result*10 + c - '0';
        }
        return result;
    }

    enum class TokenType : std::uint64_t {
        IDENTIFIER = UINT64_C(0),
        LITERAL    = UINT64_C(1) << 61,
        OPERATOR   = UINT64_C(2) << 61,
        STRUCTURE  = UINT64_C(3) << 61,
        SPECIAL    = UINT64_C(7) << 61,

        LITERAL_INT = LITERAL | 1,

        OPERATOR_PLUS  = OPERATOR | 1,
        OPERATOR_MINUS = OPERATOR | 2,
        OPERATOR_STAR  = OPERATOR | 3,
        OPERATOR_SLASH = OPERATOR | 4,
        OPERATOR_GT    = OPERATOR | 5,
        OPERATOR_LT    = OPERATOR | 6,
        OPERATOR_GEQ   = OPERATOR | 7,
        OPERATOR_LEQ   = OPERATOR | 8,
        OPERATOR_EQ    = OPERATOR | 9,
        OPERATOR_NEQ   = OPERATOR | 10,

        EQUALS      = STRUCTURE | 1,
        COLON       = STRUCTURE | 2,
        SEMICOLON   = STRUCTURE | 3,
        ARROW       = STRUCTURE | 4,
        OPEN_CURLY  = STRUCTURE | 5,
        CLOSE_CURLY = STRUCTURE | 6,
        OPEN_ROUND  = STRUCTURE | 7,
        CLOSE_ROUND = STRUCTURE | 8,

        END_OF_INPUT  = SPECIAL | 1,
        INVALID_INPUT = SPECIAL | 2,
    };

    struct Token {
        TokenType type;
        std::string_view value;
    };

    Token read_token(std::string_view& code) {
        skip_whitespace(code);
        if (code.empty()) {
            return {TokenType::END_OF_INPUT, code};
        }
        if (isdigit(code[0])) {
            return {TokenType::LITERAL_INT, read_number(code)};
        }
        if (isalpha(code[0])) {
            return {TokenType::IDENTIFIER, read_identifier(code)};
        }
        switch (code[0]) {
        case '+':
            return {TokenType::OPERATOR_PLUS, read_chars(code, 1)};
        case '-':
            return {TokenType::OPERATOR_MINUS, read_chars(code, 1)};
        case '*':
            return {TokenType::OPERATOR_STAR, read_chars(code, 1)};
        case '/':
            return {TokenType::OPERATOR_SLASH, read_chars(code, 1)};
        case '{':
            return {TokenType::OPEN_CURLY, read_chars(code, 1)};
        case '}':
            return {TokenType::CLOSE_CURLY, read_chars(code, 1)};
        case '(':
            return {TokenType::OPEN_ROUND, read_chars(code, 1)};
        case ')':
            return {TokenType::CLOSE_ROUND, read_chars(code, 1)};
        case ':':
            return {TokenType::COLON, read_chars(code, 1)};
        case '=':
            return code.size() > 1 && code[1] == '=' ?
                Token{TokenType::OPERATOR_EQ, read_chars(code, 2)} :
                Token{TokenType::EQUALS, read_chars(code, 1)};
        case '!':
            return code.size() > 1 && code[1] == '=' ?
                Token{TokenType::OPERATOR_NEQ, read_chars(code, 2)} :
                Token{TokenType::INVALID_INPUT, code};
        case '>':
            return code.size() > 1 && code[1] == '=' ?
                Token{TokenType::OPERATOR_GEQ, read_chars(code, 2)} :
                Token{TokenType::OPERATOR_GT, read_chars(code, 1)};
        case '<':
            return code.size() > 1 && code[1] == '=' ?
                Token{TokenType::OPERATOR_LEQ, read_chars(code, 2)} :
                Token{TokenType::OPERATOR_LT, read_chars(code, 1)};
        }
        return {TokenType::INVALID_INPUT, code};
    }

    using Scope = std::unordered_map<std::string_view, grlang::node::Node::Ptr>;

    struct Parser {
        std::string_view code;
        Token next_token;

        Parser(std::string_view code_) : code(code_) {
            read_next_token();
        }

        void read_next_token() {
            next_token = ::read_token(code);
        }
    };

    grlang::node::Node::Type operation_type(TokenType token) {
        switch (token)
        {
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
        switch (type)
        {
        case grlang::node::Node::Type::DATA_OP_NEG:
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

    int (*op_func(grlang::node::Node::Type type))(int, int) {
        switch (type)
        {
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

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type, int value, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        auto result = std::make_shared<grlang::node::Node>(type, inputs);
        result->value_int = value;
        return result;
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        return std::make_shared<grlang::node::Node>(type, inputs);
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type, int value) {
        return make_node(type, value, {});
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type) {
        return std::make_shared<grlang::node::Node>(type);
    }

    grlang::node::Node::Ptr peephole(grlang::node::Node::Ptr node) {
        if (node->type >= grlang::node::Node::Type::DATA_OP_ADD
                && node->type <= grlang::node::Node::Type::DATA_OP_NEQ
                && node->inputs.at(0)->type == grlang::node::Node::Type::DATA_TERM
                && node->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM) {
            return make_node(grlang::node::Node::Type::DATA_TERM, op_func(node->type)(node->inputs.at(0)->value_int, node->inputs.at(1)->value_int));
        }
        switch (node->type)
        {
        case grlang::node::Node::Type::DATA_OP_NEG:
            if (node->inputs.at(0)->type == grlang::node::Node::Type::DATA_TERM) {
                return make_node(grlang::node::Node::Type::DATA_TERM, -node->inputs.at(0)->value_int);
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
        case grlang::node::Node::Type::CONTROL_IFELSE:
            break;
        case grlang::node::Node::Type::CONTROL_PROJECT:
            if (node->inputs.at(0)->type == grlang::node::Node::Type::CONTROL_IFELSE && node->inputs.at(0)->inputs.at(1)->type == grlang::node::Node::Type::DATA_TERM) {
                if ((node->inputs.at(0)->inputs.at(1)->value_int==1) == (node->value_int==0)) {
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

    grlang::node::Node::Ptr make_peep_node(grlang::node::Node::Type type, int value, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        return peephole(make_node(type, value, inputs));
    }

    grlang::node::Node::Ptr parse_expression(Parser& parser, const Scope& scope, std::uint8_t prev_precedence) {
        grlang::node::Node::Ptr result;
        switch (parser.next_token.type) {
            case TokenType::OPERATOR_MINUS: {
                parser.read_next_token();
                auto precedence = operation_precedence(grlang::node::Node::Type::DATA_OP_NEG);
                result = make_peep_node(grlang::node::Node::Type::DATA_OP_NEG, {parse_expression(parser, scope, precedence)});
                break;
            }
            case TokenType::IDENTIFIER:
                result = scope.at(parser.next_token.value);
                parser.read_next_token();
                break;
            case TokenType::LITERAL_INT:
                result = make_node(grlang::node::Node::Type::DATA_TERM, svtoi(parser.next_token.value));
                parser.read_next_token();
                break;
            default:
                throw std::runtime_error("Expected operand!");
        }

        while ((static_cast<std::uint64_t>(parser.next_token.type) & (UINT64_C(7) << 61)) == static_cast<std::uint64_t>(TokenType::OPERATOR)) {
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

    void merge_scope(const Scope& src, Scope& dst) {
        for (auto& [key, val] : dst) {
            val = src.at(key);
        }
    }

    void merge_scopes(const Scope& src1, const Scope& src2, Scope& dst, const grlang::node::Node::Ptr& region) {
        for (auto& [key, val] : dst) {
            if (src1.at(key) != src2.at(key)) {  // TODO: make into a value comparison
                val = make_peep_node(grlang::node::Node::Type::DATA_PHI, {region, src1.at(key), src2.at(key)});
            } else {
                val = src1.at(key);
            }
        }
    }

    void parse_statement(Parser& parser, Scope& scope, const grlang::node::Node::Ptr& stop);

    void parse_ifelse(Parser& parser, Scope& scope, const grlang::node::Node::Ptr& stop)
    {
        assert(parser.next_token.value == "if");
        parser.read_next_token();
        auto condition = parse_expression(parser, scope, 255);
        auto ifelse = make_peep_node(grlang::node::Node::Type::CONTROL_IFELSE, {scope.at("$ctl"), condition});
        
        auto true_proj = make_peep_node(grlang::node::Node::Type::CONTROL_PROJECT, 0, {ifelse});
        auto true_scope = scope;
        true_scope["$ctl"] = true_proj;
        parse_statement(parser, true_scope, stop);
        
        auto false_proj = make_peep_node(grlang::node::Node::Type::CONTROL_PROJECT, 1, {ifelse});
        auto false_scope = scope;
        false_scope["$ctl"] = false_proj;
        if (parser.next_token.value == "else")
        {
            parser.read_next_token();
            parse_statement(parser, false_scope, stop);
        }
        
        auto region = make_node(grlang::node::Node::Type::CONTROL_REGION, {nullptr, true_proj, false_proj}); // dont peephole region before phis
        merge_scopes(true_scope, false_scope, scope, region);
        scope["$ctl"] = peephole(region);
        return;
    }

    void parse_block(Parser& parser, Scope& scope, const grlang::node::Node::Ptr& stop) {
        while (parser.next_token.type != TokenType::CLOSE_CURLY && parser.next_token.type != TokenType::END_OF_INPUT)
        {
            parse_statement(parser, scope, stop);
        }
    }

    void parse_statement(Parser& parser, Scope& scope, const grlang::node::Node::Ptr& stop)
    {
        switch (parser.next_token.type)
        {
        case TokenType::END_OF_INPUT:
            break;
        case TokenType::OPEN_CURLY: {
            parser.read_next_token();
            auto new_scope = scope;
            parse_block(parser, new_scope, stop);
            if (parser.next_token.type != TokenType::CLOSE_CURLY) {
                throw std::runtime_error("Expected }");
            }
            merge_scope(new_scope, scope);
            parser.read_next_token();
            break;
        }
        case TokenType::IDENTIFIER: {
            if (parser.next_token.value == "return") {
                parser.read_next_token();
                auto result = make_peep_node(grlang::node::Node::Type::CONTROL_RETURN, {scope.at("$ctl"), parse_expression(parser, scope, 255)});
                stop->inputs.push_back(result);
                break;
            }
            if (parser.next_token.value == "if") {
                parse_ifelse(parser, scope, stop);
                break;
            }

            auto name = parser.next_token.value;
            // TODO: check name is not a keyword
            parser.read_next_token();
            if (parser.next_token.type == TokenType::COLON) {
                if (scope.contains(name)) {
                    throw std::runtime_error("already defined");
                }
                parser.read_next_token();
                assert(parser.next_token.value == "int");   // TODO: read type
                parser.read_next_token();
            } else if (!scope.contains(name)) {
                throw std::runtime_error("not defined");
            }
            if (parser.next_token.type != TokenType::EQUALS) {
                throw std::runtime_error("Expected assignment");
            }
            parser.read_next_token();
            scope[name] = parse_expression(parser, scope, 255);
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
    auto start = make_node(grlang::node::Node::Type::CONTROL_START);
    scope["$ctl"] = start;
    auto stop = make_node(grlang::node::Node::Type::CONTROL_STOP);
    parse_block(parser, scope, stop);
    return stop;
}
