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

    std::string_view read_chars(std::string_view& code, int n) {
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
        IDENTIFIER = 0ull,
        LITERAL    = 1ull << 61,
        OPERATOR   = 2ull << 61,
        STRUCTURE  = 3ull << 61,
        SPECIAL    = 7ull << 61,

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
        using grlang::node::Node;

        switch (type)
        {
        case Node::Type::DATA_OP_NEG:
            return 2;
        case Node::Type::DATA_OP_MUL:
        case Node::Type::DATA_OP_DIV:
            return 3;
        case Node::Type::DATA_OP_ADD:
        case Node::Type::DATA_OP_SUB:
            return 4;
        case Node::Type::DATA_OP_GT:
        case Node::Type::DATA_OP_GEQ:
        case Node::Type::DATA_OP_LT:
        case Node::Type::DATA_OP_LEQ:
            return 6;
        case Node::Type::DATA_OP_EQ:
        case Node::Type::DATA_OP_NEQ:
            return 7;
        default:
            return 255;
        }
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type) {
        return std::make_shared<grlang::node::Node>(type);
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        return std::make_shared<grlang::node::Node>(type, inputs);
    }

    grlang::node::Node::Ptr make_node(grlang::node::Node::Type type, int value, std::initializer_list<grlang::node::Node::Ptr> inputs) {
        auto result = make_node(type, inputs);
        result->value_int = value;
        return result;
    }

    grlang::node::Node::Ptr parse_expression(Parser& parser, const Scope& scope, std::uint8_t prev_precedence) {
        grlang::node::Node::Ptr result;
        switch (parser.next_token.type) {
            case TokenType::OPERATOR_MINUS:
            {
                parser.read_next_token();
                result = make_node(grlang::node::Node::Type::DATA_OP_NEG,
                    {parse_expression(parser, scope, operation_precedence(grlang::node::Node::Type::DATA_OP_NEG))});
            }
            break;
            case TokenType::IDENTIFIER:
            {
                result = scope.at(parser.next_token.value);
                parser.read_next_token();
            }
            break;
            case TokenType::LITERAL_INT:
            {
                result = make_node(grlang::node::Node::Type::DATA_TERM);
                result->value_int = svtoi(parser.next_token.value);
                parser.read_next_token();
            }
            break;
            default:
                throw std::runtime_error("Expected operand!");
        }

        while (true) {
            if ((static_cast<std::uint64_t>(parser.next_token.type) & (7ull << 61)) != static_cast<std::uint64_t>(TokenType::OPERATOR)) {
                return result;
            }
            auto node_type = operation_type(parser.next_token.type);
            auto precedence = operation_precedence(node_type);
            if (prev_precedence < precedence) {
                return result;
            }
            parser.read_next_token();
            result = make_node(node_type, {result, parse_expression(parser, scope, precedence)});
        }
    }

    void merge_scope(const Scope& src, Scope& dst) {
        for (auto& [key, val] : dst) {
            val = src.at(key);
        }
    }

    void merge_scopes(const Scope& src1, const Scope& src2, Scope& dst, const grlang::node::Node::Ptr& region) {
        for (auto& [key, val] : dst) {
            // TODO: make into a value comparison
            if (src1.at(key) != src2.at(key)) {
                val = make_node(grlang::node::Node::Type::DATA_PHI, {region, src1.at(key), src2.at(key)});
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
        auto ifelse = make_node(grlang::node::Node::Type::CONTROL_IFELSE, {scope.at("$ctl"), condition});
        
        auto true_proj = make_node(grlang::node::Node::Type::CONTROL_PROJECT, 0, {ifelse});
        auto true_scope = scope;
        true_scope["$ctl"] = true_proj;
        parse_statement(parser, true_scope, stop);
        
        auto false_proj = make_node(grlang::node::Node::Type::CONTROL_PROJECT, 1, {ifelse});
        auto false_scope = scope;
        false_scope["$ctl"] = false_proj;
        if (parser.next_token.value == "else")
        {
            parser.read_next_token();
            parse_statement(parser, false_scope, stop);
        }
        
        auto region = make_node(grlang::node::Node::Type::CONTROL_REGION, {nullptr, true_proj, false_proj});
        merge_scopes(true_scope, false_scope, scope, region);
        scope["$ctl"] = region;
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
            return;
        case TokenType::OPEN_CURLY:
        {
            parser.read_next_token();
            auto new_scope = scope;
            parse_block(parser, new_scope, stop);
            if (parser.next_token.type != TokenType::CLOSE_CURLY) {
                throw std::runtime_error("Expected }");
            }
            merge_scope(new_scope, scope);
            parser.read_next_token();
            return;
        }
        case TokenType::IDENTIFIER:
        {
            if (parser.next_token.value == "return") {
                parser.read_next_token();
                auto result = make_node(grlang::node::Node::Type::CONTROL_RETURN, {scope.at("$ctl"), parse_expression(parser, scope, 255)});
                stop->inputs.push_back(result);
                return;
            }
            if (parser.next_token.value == "if") {
                parse_ifelse(parser, scope, stop);
                return;
            }

            auto name = parser.next_token.value;
            // TODO: check name is not a keyword
            parser.read_next_token();
            if (parser.next_token.type == TokenType::COLON) {
                if (scope.contains(name)) {
                    throw std::runtime_error("already defined");
                }
                parser.read_next_token();
                // TODO: read type
                parser.read_next_token();
            } else if (!scope.contains(name)) {
                throw std::runtime_error("not defined");
            }
            if (parser.next_token.type != TokenType::EQUALS) {
                throw std::runtime_error("Expected assignment");
            }
            parser.read_next_token();
            scope[name] = parse_expression(parser, scope, 255);
            return;
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
