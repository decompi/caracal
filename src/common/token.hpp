#pragma once

#include <string>

enum class TokenKind {
    // special tkns
    Eof,
    Invalid,

    // literals
    Integer,

    // identifiers
    Identifier,

    // keywords
    Fn,
    Let,
    If,
    Else,
    While,
    Return,
    I32,
    F64,
    Bool,
    True,
    False,

    // punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,

    Colon,
    Semicolon,
    Comma,

    // operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Equal,
    Bang,
    EqualEqual,
    BangEqual,
    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Arrow
};

struct Token {
    TokenKind kind;
    std::string lexeme;
    int line;
    int column;
};

std::string tokenKindToString(TokenKind kind);