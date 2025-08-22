#include "grlang/detail/token.h"


namespace {
    std::string_view::size_type count_class(const std::string_view& code, int (*isclass)(int)) {
        std::string_view::size_type n = 0;
        while (n<code.size() && isclass(code[n])) {
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
}

namespace grlang::parse::detail {
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
            return code.size() > 1 && code[1] == '>' ?
                Token{TokenType::ARROW, read_chars(code, 2)} :
                Token{TokenType::OPERATOR_MINUS, read_chars(code, 1)};
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
        case ',':
            return {TokenType::COMMA, read_chars(code, 1)};
        case ':':
            return code.size() > 1 && code[1] == '=' ?
                Token{TokenType::DECLARE_AUTO, read_chars(code, 2)} :
                Token{TokenType::DECLARE_TYPE, read_chars(code, 1)};
        case '=':
            return code.size() > 1 && code[1] == '=' ?
                Token{TokenType::OPERATOR_EQ, read_chars(code, 2)} :
                Token{TokenType::REBIND, read_chars(code, 1)};
        case '!':
            return code.size() > 1 && code[1] == '=' ?
                Token{TokenType::OPERATOR_NEQ, read_chars(code, 2)} :
                Token{TokenType::OPERATOR_NOT, read_chars(code, 1)};
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
}
