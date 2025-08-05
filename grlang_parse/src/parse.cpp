#include <stdexcept>
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

    enum class TokenType : u_int64_t {
        IDENTIFIER = 0ull,
        KEYWORD    = 1ull << 61,
        LITERAL    = 2ull << 61,
        OPERATOR   = 3ull << 61,
        STRUCTURE  = 4ull << 61,
        SPECIAL    = 7ull << 61,

        KEYWORD_RETURN = KEYWORD | 1,

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

        ASSIGNMENT          = STRUCTURE | 1,
        STRUCTURE_NEW_SCOPE = STRUCTURE | 2,
        STRUCTURE_END_SCOPE = STRUCTURE | 3,

        END_OF_INPUT  = SPECIAL | 1,
        INVALID_INPUT = SPECIAL | 2,
    };

    struct Token {
        TokenType type;
        std::string_view value;
    };

    Token check_keyword(Token token) {
        if (token.value == "return") {
            return {TokenType::KEYWORD_RETURN, token.value};
        }
        return token;
    }

    Token read_token(std::string_view& code) {
        skip_whitespace(code);
        if (code.empty()) {
            return {TokenType::END_OF_INPUT, code};
        }
        if (isdigit(code[0])) {
            return {TokenType::LITERAL_INT, read_number(code)};
        }
        if (isalpha(code[0])) {
            return check_keyword({TokenType::IDENTIFIER, read_identifier(code)});
        }
        if (code[0] == '+') {
            return {TokenType::OPERATOR_PLUS, read_chars(code, 1)};
        }
        if (code[0] == '-') {
            return {TokenType::OPERATOR_MINUS, read_chars(code, 1)};
        }
        if (code[0] == '*') {
            return {TokenType::OPERATOR_STAR, read_chars(code, 1)};
        }
        if (code[0] == '/') {
            return {TokenType::OPERATOR_SLASH, read_chars(code, 1)};
        }
        if (code[0] == '/') {
            return {TokenType::OPERATOR_SLASH, read_chars(code, 1)};
        }
        if (code[0] == '/') {
            return {TokenType::OPERATOR_SLASH, read_chars(code, 1)};
        }
        if (code.size() < 2) {
            return {TokenType::END_OF_INPUT, code};
        }
        if (code[0] == '=') {
            if (code[1] == '=') {
                return {TokenType::OPERATOR_EQ, read_chars(code, 2)};
            } else {
                return {TokenType::ASSIGNMENT, read_chars(code, 1)};
            }
        }
        if (code[0] == '!' && code[1] == '=') {
            return {TokenType::OPERATOR_NEQ, read_chars(code, 2)};
        }
        if (code[0] == '>') {
            if (code[1] == '=') {
                return {TokenType::OPERATOR_GEQ, read_chars(code, 2)}; 
            } else {
                return {TokenType::OPERATOR_GT, read_chars(code, 1)};
            }
        }
        if (code[0] == '<') {
            if (code[1] == '=') {
                return {TokenType::OPERATOR_LEQ, read_chars(code, 2)}; 
            } else {
                return {TokenType::OPERATOR_LT, read_chars(code, 1)};
            }
        }
        return {TokenType::INVALID_INPUT, code};
    }

    struct Parser {
        std::string_view code;
        Token next_token;
        std::vector<std::unordered_map<std::string_view, std::shared_ptr<grlang::node::Node>>> scopes;

        Parser(std::string_view code_) : code(code_) {
            new_scope();
            advance();
        }

        void advance() {
            next_token = ::read_token(code);
        }

        void new_scope() {
            scopes.emplace_back();
        }

        void end_scope() {
            scopes.pop_back();
        }
    };

    grlang::node::NodeType operation_type(TokenType token) {
        switch (token)
        {
        case TokenType::OPERATOR_PLUS:
            return grlang::node::NodeType::OPERATION_ADD;
        case TokenType::OPERATOR_MINUS:
            return grlang::node::NodeType::OPERATION_SUB;
        case TokenType::OPERATOR_STAR:
            return grlang::node::NodeType::OPERATION_MUL;
        case TokenType::OPERATOR_SLASH:
            return grlang::node::NodeType::OPERATION_DIV;
        case TokenType::OPERATOR_LT:
            return grlang::node::NodeType::OPERATION_LT;
        case TokenType::OPERATOR_GT:
            return grlang::node::NodeType::OPERATION_GT;
        case TokenType::OPERATOR_LEQ:
            return grlang::node::NodeType::OPERATION_LEQ;
        case TokenType::OPERATOR_GEQ:
            return grlang::node::NodeType::OPERATION_GEQ;
        case TokenType::OPERATOR_EQ:
            return grlang::node::NodeType::OPERATION_EQ;
        case TokenType::OPERATOR_NEQ:
            return grlang::node::NodeType::OPERATION_NEQ;
        default:
            throw std::runtime_error("Unsupported token type");
        }
    }

    u_int8_t operation_precedence(grlang::node::NodeType type) {
        using grlang::node::NodeType;

        switch (type)
        {
        case NodeType::OPERATION_NEG:
            return 2;
        case NodeType::OPERATION_MUL:
        case NodeType::OPERATION_DIV:
            return 3;
        case NodeType::OPERATION_ADD:
        case NodeType::OPERATION_SUB:
            return 4;
        case NodeType::OPERATION_GT:
        case NodeType::OPERATION_GEQ:
        case NodeType::OPERATION_LT:
        case NodeType::OPERATION_LEQ:
            return 6;
        case NodeType::OPERATION_EQ:
        case NodeType::OPERATION_NEQ:
            return 7;
        default:
            return 255;
        }
    }

    std::shared_ptr<grlang::node::Node> parse_expression(Parser& parser, uint8_t prev_precedence) {
        std::shared_ptr<grlang::node::Node> result;
        switch (parser.next_token.type) {
            case TokenType::OPERATOR_MINUS:
            {
                parser.advance();
                result = std::make_shared<grlang::node::Node>(
                    grlang::node::NodeType::OPERATION_NEG,
                    std::vector<std::shared_ptr<grlang::node::Node>>{parse_expression(parser, operation_precedence(grlang::node::NodeType::OPERATION_NEG))});
            }
            break;
            case TokenType::IDENTIFIER:
            {
                result = parser.scopes.back().at(parser.next_token.value);
                parser.advance();
            }
            break;
            case TokenType::LITERAL_INT:
            {
                result = std::make_shared<grlang::node::Node>(grlang::node::NodeType::CONSTANT_INT);
                result->value_int = svtoi(parser.next_token.value);
                parser.advance();
            }
            break;
            default:
                throw std::runtime_error("Expected operand!");
        }

        while (true) {
            if ((static_cast<uint64_t>(parser.next_token.type) & (7ull << 61)) != static_cast<uint64_t>(TokenType::OPERATOR)) {
                return result;
            }
            auto node_type = operation_type(parser.next_token.type);
            auto precedence = operation_precedence(node_type);
            if (prev_precedence < precedence) {
                return result;
            }
            parser.advance();
            result = std::make_shared<grlang::node::Node>(
                node_type,
                std::vector<std::shared_ptr<grlang::node::Node>>{result, parse_expression(parser, precedence)});
        }
    }

    std::shared_ptr<grlang::node::Node> parse_declaration(Parser& parser) {
        return nullptr;
    }

    std::shared_ptr<grlang::node::Node> parse_statement(Parser& parser) {
        switch (parser.next_token.type)
        {
        case TokenType::END_OF_INPUT:
            return nullptr;
        case TokenType::KEYWORD_RETURN:
        {
            parser.advance();
            auto result = std::make_shared<grlang::node::Node>(grlang::node::NodeType::RETURN);
            result->inputs.push_back(parse_expression(parser, 255));
            return result;
        }
        case TokenType::IDENTIFIER:
        {
            auto name = parser.next_token.value;
            parser.advance();
            if (parser.next_token.type != TokenType::ASSIGNMENT) {
                throw std::runtime_error("Expected assignment");
            }
            parser.advance();
            auto result = parse_expression(parser, 255);
            parser.scopes.back()[name] = result;
            return result;
        }
        default:
            throw std::runtime_error("Unexpected token");
        }
    }

    std::shared_ptr<grlang::node::Node> parse_block(Parser& parser) {
        std::shared_ptr<grlang::node::Node> result;
        while (parser.next_token.type != TokenType::END_OF_INPUT)
        {
            result = parse_statement(parser);
        }
        return result;
    }
}

std::shared_ptr<grlang::node::Node> grlang::parse::parse(std::string_view code) {
    Parser parser(code);
    return parse_block(parser);
}
