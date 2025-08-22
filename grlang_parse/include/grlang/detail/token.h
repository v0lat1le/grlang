#pragma once

#include <cstdint>
#include <string>


namespace grlang::parse::detail {
    enum class TokenType : std::uint8_t {
        IDENTIFIER,
        LITERAL_INT,

        META_BINARY_BEGIN,  // NOTE: put binary operation tags between META_BINARY_BEGIN and META_BINARY_END
        OPERATOR_PLUS,
        OPERATOR_MINUS,
        OPERATOR_STAR,
        OPERATOR_SLASH,
        OPERATOR_GT,
        OPERATOR_LT,
        OPERATOR_GEQ,
        OPERATOR_LEQ,
        OPERATOR_EQ,
        OPERATOR_NEQ,
        META_BINARY_END,
        OPERATOR_NOT,

        DECLARE_TYPE,
        DECLARE_AUTO,
        REBIND,

        SEMICOLON,
        ARROW,

        OPEN_CURLY,
        CLOSE_CURLY,
        OPEN_ROUND,
        CLOSE_ROUND,
        OPEN_SQUARE,
        CLOSE_SQUARE,

        END_OF_INPUT,
        INVALID_INPUT,
    };

    struct Token {
        TokenType type;
        std::string_view value;
    };

    Token read_token(std::string_view& code);
}
