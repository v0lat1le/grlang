#include <stdexcept>

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

    enum class TokenType {
        IDENTIFIER,
        LITERAL_INT,
        OPERATION_NIL,
        OPERATION_ADD,
        OPERATION_MUL,
        KEYWORD_RETURN,
        END_OF_INPUT,
        INVALID_INPUT,
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
            return {TokenType::OPERATION_ADD, read_chars(code, 1)};
        }
        if (code[0] == '*') {
            return {TokenType::OPERATION_MUL, read_chars(code, 1)};
        }
        return {TokenType::INVALID_INPUT, code};
    }

    struct Tokenizer {
        Token next_token;
        std::string_view code;

        Tokenizer(std::string_view code_) : code(code_) {
            advance();
        }

        void advance() {
            next_token = ::read_token(code);
        }
    };

    std::shared_ptr<grlang::node::Node> parse_expression(Tokenizer& tokenizer, TokenType prev_op) {
        std::shared_ptr<grlang::node::Node> result;
        switch (tokenizer.next_token.type) {
            case TokenType::LITERAL_INT:
            {
                result = std::make_shared<grlang::node::Node>(grlang::node::NodeType::CONSTANT_INT);
                result->value_int = svtoi(tokenizer.next_token.value);
                tokenizer.advance();
            }
            break;
            default:
                throw std::runtime_error("Expected operand!");
        }

        while (true) {
            switch (tokenizer.next_token.type) {
                case TokenType::OPERATION_ADD:
                {
                    if (prev_op > tokenizer.next_token.type) {
                        return result;
                    }

                    tokenizer.advance();
                    result = std::make_shared<grlang::node::Node>(
                        grlang::node::NodeType::OPERATION_ADD,
                        std::vector<std::shared_ptr<grlang::node::Node>>{result, parse_expression(tokenizer, TokenType::OPERATION_ADD)});
                }
                break;
                case TokenType::OPERATION_MUL:
                {
                    if (prev_op > tokenizer.next_token.type) {
                        return result;
                    }

                    tokenizer.advance();
                    result = std::make_shared<grlang::node::Node>(
                        grlang::node::NodeType::OPERATION_MUL,
                        std::vector<std::shared_ptr<grlang::node::Node>>{result, parse_expression(tokenizer, TokenType::OPERATION_MUL)});
                }
                break;
                default:
                    return result;
            }
        }
    }

    std::shared_ptr<grlang::node::Node> parse_declaration(Tokenizer& tokenizer) {
        return nullptr;
    }
}

std::shared_ptr<grlang::node::Node> grlang::parse::parse(std::string_view code) {
    Tokenizer tokenizer(code);
    while(true) {
        switch (tokenizer.next_token.type)
        {
        case TokenType::END_OF_INPUT:
            return nullptr;
        case TokenType::KEYWORD_RETURN:
        {
            tokenizer.advance();
            auto result = std::make_shared<grlang::node::Node>(grlang::node::NodeType::RETURN);
            result->inputs.push_back(parse_expression(tokenizer, TokenType::OPERATION_NIL));
            return result;
        }
        default:
            return parse_declaration(tokenizer);
        }
    }
}
