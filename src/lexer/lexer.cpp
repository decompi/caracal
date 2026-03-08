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
            continue;
        }

        if(c == '/' && peekNext() == '/') {
            while(!isAtEnd() && peek() != '\n') {
                advance();
            }
            continue;
        }

        break;
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

    return makeToken(
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
        return makeToken(TokenKind::Fn, text, startLine, startColumn);
    }
    if(text == "let") {
        return makeToken(TokenKind::Let, text, startLine, startColumn);
    }
    if(text == "if") {
        return makeToken(TokenKind::If, text, startLine, startColumn);
    }
    if(text == "else") {
        return makeToken(TokenKind::Else, text, startLine, startColumn);
    }
    if(text == "while") {
        return makeToken(TokenKind::While, text, startLine, startColumn);
    }
    if(text == "return") {
        return makeToken(TokenKind::Return, text, startLine, startColumn);
    }
    if(text == "i32") {
        return makeToken(TokenKind::I32, text, startLine, startColumn);
    }

    return makeToken(TokenKind::Identifier, text, startLine, startColumn);
}


// just a test function to print each tokenkind in the code
Token Lexer::nextToken() {
    skipWhitespace();

    int startLine = line_;
    int startColumn = column_;

    if(isAtEnd()) {
        return makeToken(TokenKind::Eof, "", startLine, startColumn);
    }

    char c = peek();

    // get the full word or number
    if(isDigit(c)) {
        return number();
    } else if(isAlpha(c)) {
        return identifierOrKeyword();
    }

    advance();

    switch(c) {
        case '(':
            return makeToken(TokenKind::LParen, "(", startLine, startColumn);
        case ')':
            return makeToken(TokenKind::RParen, ")", startLine, startColumn);
        case '{':
            return makeToken(TokenKind::LBrace, "{", startLine, startColumn);
        case '}':
            return makeToken(TokenKind::RBrace, "}", startLine, startColumn);
        case ':':
            return makeToken(TokenKind::Colon, ":", startLine, startColumn);
        case ';':
            return makeToken(TokenKind::Semicolon, ";", startLine, startColumn);
        case ',':
            return makeToken(TokenKind::Comma, ",", startLine, startColumn);
        
        case '+':
            return makeToken(TokenKind::Plus, "+", startLine, startColumn);
        case '*':
            return makeToken(TokenKind::Star, "-", startLine, startColumn);
        case '/':
            return makeToken(TokenKind::Slash, "/", startLine, startColumn);
        case '&':
            return makeToken(TokenKind::Percent, "&", startLine, startColumn);
        
        case '-': 
            if(match('>')) {
                return makeToken(TokenKind::Arrow, "->", startLine, startColumn);
            }
            return makeToken(TokenKind::Minus, "-", startLine, startColumn);
        case '=':
            if(match('=')) {
                return makeToken(TokenKind::EqualEqual, "==", startLine, startColumn);
            }
            return makeToken(TokenKind::Equal, "=", startLine, startColumn);
        case '!':
            if(match('=')) {
                return makeToken(TokenKind::BangEqual, "!=", startLine, startColumn);
            }
            return makeToken(TokenKind::Invalid, "!", startLine, startColumn);
        case '<':
            if(match('=')) {
                return makeToken(TokenKind::LessEqual, "<=", startLine, startColumn);
            }
            return makeToken(TokenKind::Less, "<", startLine, startColumn);
        case '>':
            if(match('=')) {
                return makeToken(TokenKind::GreaterEqual, ">=", startLine, startColumn);
            }
            return makeToken(TokenKind::Greater, ">", startLine, startColumn);

        default:
            return makeToken(TokenKind::Invalid, std::string(1,c), startLine, startColumn);
              
    }
}