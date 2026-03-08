#pragma once
#include <string>
#include "../common/token.hpp"

struct Lexer {
    explicit Lexer(const std::string &source);
    Token nextToken();
private: 
    std::string source_;
    std::size_t pos_;

    int line_;
    int column_;

    char peek() const;
    char peekNext() const;
    char advance();
    bool match(char expected);
    bool isAtEnd() const;

    void skipWhitespace();

    Token makeToken(TokenKind kind, const std::string &lexeme, int line, int column) const;
    Token number();
    Token identifierOrKeyword();

    static bool isDigit(char c);
    static bool isAlpha(char c);
    static bool isAlphaNumeric(char c);
};