#include "lexer.hpp"

const char DEFAULT_RETURN = '\0';

Lexer::Lexer(const std::string &source) : source_(source), pos_(0), line_(1), column_(1) {
    // emnty on purpose
}

char Lexer::peek() const {
    if(isAtEnd()) {
        return DEFAULT_RETURN;
    }
    return source_[pos_];
}

// look ahead 2 spots
// useful for things like ==, !=.. etc
char Lexer::peekNext() const {
    if(pos_ + 1 >= source_.length()) {
        return DEFAULT_RETURN;
    }

    return source_[pos_ + 1];
}

// move on to the next char
char Lexer::advance() {
    if(isAtEnd()) {
        return DEFAULT_RETURN;
    }

    char c = source_[pos_++];
    if(c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }

    return c;
}

// check if the next char is what we expect
// useful for == etc
bool Lexer::match(char expected) {
    if(isAtEnd()) {
        return false;
    }
    
    if(source_[pos_] != expected) {
        return false;
    }

    advance();
    return true;
}

// end of source?
bool Lexer::isAtEnd() const {
    bool endCheck = pos_ >= source_.length();
    return endCheck;
}

bool Lexer::isDigit(char c) {
    bool digitCheck = c >= '0' && c <= '9';
    return digitCheck;
}

bool Lexer::isAlpha(char c) {
    bool alphaCheck = (c == '_') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
    return alphaCheck;
}

bool Lexer::isAlphaNumeric(char c) {
    return isAlpha(c) || isDigit(c);
}

void Lexer::skipWhitespace() {
    while(!isAtEnd()) {
        char c = peek();
        if(c == ' ' || c == '\n') {
            advance();
        } else {
            break;
        }
    }
}

Token Lexer::makeToken(TokenKind kind, const std::string &lexeme, int line, int column) const {
    return Token {
        kind,
        lexeme,
        line,
        column
    };
}

Token Lexer::number() {
    int startLine = line_;
    int startColumn = column_;

    std::size_t startPos = pos_;

    while(isDigit(peek())) {
        advance();
    }

    return Lexer::makeToken(
        TokenKind::Integer,
        source_.substr(startPos, pos_ - startPos),
        startLine,
        startColumn
    );
}

Token Lexer::identifierOrKeyword() {
    int startLine = line_;
    int startColumn = column_;
    std::size_t startPos = pos_;

    while(isAlphaNumeric(peek())) {
        advance();
    }

    std::string text = source_.substr(startPos, pos_ - startPos);

    if(text == "fn") {
        return Lexer::makeToken(TokenKind::Fn, text, startLine, startColumn);
    }
    if(text == "let") {
        return Lexer::makeToken(TokenKind::Let, text, startLine, startColumn);
    }
    if(text == "if") {
        return Lexer::makeToken(TokenKind::If, text, startLine, startColumn);
    }
    if(text == "else") {
        return Lexer::makeToken(TokenKind::Else, text, startLine, startColumn);
    }
    if(text == "while") {
        return Lexer::makeToken(TokenKind::While, text, startLine, startColumn);
    }
    if(text == "return") {
        return Lexer::makeToken(TokenKind::Return, text, startLine, startColumn);
    }
    if(text == "i32") {
        return Lexer::makeToken(TokenKind::I32, text, startLine, startColumn);
    }

    return Lexer::makeToken(TokenKind::Identifier, text, startLine, startColumn);
}


// just a test function to print each tokenkind in the code
Token Lexer::nextToken() {
    skipWhitespace();

    int startLine = line_;
    int startColumn = column_;

    if(isAtEnd()) {
        return Lexer::makeToken(TokenKind::Eof, "", startLine, startColumn);
    }

    char c = peek();

    // get the full word or number
    if(isDigit(c)) {
        return Lexer::number();
    } else if(isAlpha(c)) {
        return Lexer::identifierOrKeyword();
    }

    advance();

    switch(c) {
        case '(':
            return Lexer::makeToken(TokenKind::LParen, "(", startLine, startColumn);
        case ')':
            return Lexer::makeToken(TokenKind::RParen, ")", startLine, startColumn);
        case '{':
            return Lexer::makeToken(TokenKind::LBrace, "{", startLine, startColumn);
        case '}':
            return Lexer::makeToken(TokenKind::RBrace, "}", startLine, startColumn);
        case ':':
            return Lexer::makeToken(TokenKind::Colon, ":", startLine, startColumn);
        case ';':
            return Lexer::makeToken(TokenKind::Semicolon, ";", startLine, startColumn);
        case ',':
            return Lexer::makeToken(TokenKind::Comma, ",", startLine, startColumn);
        
        case '+':
            return Lexer::makeToken(TokenKind::Plus, "+", startLine, startColumn);
        case '*':
            return Lexer::makeToken(TokenKind::Star, "-", startLine, startColumn);
        case '/':
            return Lexer::makeToken(TokenKind::Slash, "/", startLine, startColumn);
        case '&':
            return Lexer::makeToken(TokenKind::Percent, "&", startLine, startColumn);
        
        case '-': 
            if(match('>')) {
                return makeToken(TokenKind::Arrow, "->", startLine, startColumn);
            }
            return makeToken(TokenKind::Minus, "-", startLine, startColumn);
        case '=':
            if(Lexer::match('=')) {
                return Lexer::makeToken(TokenKind::EqualEqual, "==", startLine, startColumn);
            }
            return Lexer::makeToken(TokenKind::Equal, "=", startLine, startColumn);
        case '!':
            if(Lexer::match('=')) {
                return Lexer::makeToken(TokenKind::BangEqual, "!=", startLine, startColumn);
            }
            return Lexer::makeToken(TokenKind::Invalid, "!", startLine, startColumn);
        case '<':
            if(Lexer::match('=')) {
                return Lexer::makeToken(TokenKind::LessEqual, "<=", startLine, startColumn);
            }
            return Lexer::makeToken(TokenKind::Less, "<", startLine, startColumn);
        case '>':
            if(Lexer::match('=')) {
                return Lexer::makeToken(TokenKind::GreaterEqual, ">=", startLine, startColumn);
            }
            return Lexer::makeToken(TokenKind::Greater, ">", startLine, startColumn);

        default:
            return Lexer::makeToken(TokenKind::Invalid, std::string(1,c), startLine, startColumn);
              
    }
}