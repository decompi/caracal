#include "parser.hpp"

#include <iostream>
#include <utility>
#include <stdexcept>

Parser::Parser(std::vector<Token> tokens) : tokens_(std::move(tokens)), current_(0) {

}

const Token &Parser::peek() const {
    return tokens_[current_];
}

const Token &Parser::previous() const {
    return tokens_[current_ - 1];
}

bool Parser::isAtEnd() const {
    return peek().kind == TokenKind::Eof;
}

bool Parser::check(TokenKind kind) const {
    if(isAtEnd()) {
        return false;
    }
    return peek().kind == kind;
}

bool Parser::match(TokenKind kind) {
    if(check(kind)) {
        advance();
        return true;
    }

    return false;
}

const Token &Parser::advance() {
    if(!isAtEnd()) {
        current_++;
    }
    return previous();
}

const Token &Parser::consume(TokenKind kind, const std::string &message) {
    if(check(kind)) {
        return advance();
    }
    errorHere(message);
}

[[noreturn]] void Parser::errorHere(const std::string &message) const {
    const Token &token = peek();
    throw std::runtime_error(
        "parse error at " + std::to_string(token.line) + ":" +
        std::to_string(token.column) + ": " + message
    );
}

ast::Program Parser::parseProgram() {
    std::vector<ast::FunctionDeclPtr> functions;

    while(!isAtEnd()) {
        functions.push_back(parseFunction());
    }

    return ast::Program(std::move(functions));
}

ast::FunctionDeclPtr Parser::parseFunction() {
    consume(TokenKind::Fn, "exepected fn");

    const Token &nameTok = consume(TokenKind::Identifier, "expected function name");
    std::string name = nameTok.lexeme;

    consume(TokenKind::LParen, "expected ( after function name");
    consume(TokenKind::RParen, "expected ) after function params");
    consume(TokenKind::Arrow, "expected -> after function param list");

    // keep the return type at i32 only for testing
    // change later with a system that can handle diff types
    // when more types get added
    consume(TokenKind::I32, "expected return type i32");

    auto body = parseBlock();

    return std::make_unique<ast::FunctionDecl>(
        name,
        std::vector<ast::Parameter>{},
        "i32",
        std::move(body)
    );
}

ast::BlockStmtPtr Parser::parseBlock() {
    consume(TokenKind::LBrace, "expected { to start block");

    std::vector<ast::StmtPtr> statements;

    while(!check(TokenKind::RBrace) && !isAtEnd()) {
        statements.push_back(parseStatement());
    }

    consume(TokenKind::RBrace, "expected } to end the blocjk");

    return std::make_unique<ast::BlockStmt>(std::move(statements));
}

// change this later to switch statements
// or use dispatch table
ast::StmtPtr Parser::parseStatement() {

    if(check(TokenKind::Let)) {
        return parseLetStmt();
    }

    /*
    if(check(TokenKind::Const)) {
        return parseConstStmt();
    }
    */

    if(check(TokenKind::Return)) {
        return parseReturnStmt();
    }
    
    // default fallback to parseExpressionStmt?

    errorHere("expected statement");
}

ast::StmtPtr Parser::parseLetStmt() {
    consume(TokenKind::Let, "expected let");

    const Token &nameTok = consume(TokenKind::Identifier, "expected a variable name");
    std::string varName = nameTok.lexeme;

    consume(TokenKind::Colon, "expecteda a ':' after the variable name");
    
    // change this when more types are added
    consume(TokenKind::I32, "expected type i32");

    consume(TokenKind::Equal, "expected a = after the variable name");

    auto inializer = parseExpression();

    consume(TokenKind::Semicolon, "expected ; after the let statement");

    return std::make_unique<ast::LetStmt>(
        varName,
        "i32",
        std::move(inializer)
    );
}

ast::StmtPtr Parser::parseReturnStmt() {
    consume(TokenKind::Return, "expected return");

    auto value = parseExpression();

    consume(TokenKind::Semicolon, "expected ; after return statement");

    return std::make_unique<ast::ReturnStmt>(std::move(value));
}

ast::ExprPtr Parser::parseExpression() {
    return parsePrimary();
}

ast::ExprPtr Parser::parsePrimary() {
    if(check(TokenKind::Integer)) {
        const Token &tok = advance();
        int value = std::stoi(tok.lexeme);
        
        return std::make_unique<ast::IntegerExpr>(value);
    }

    if(check(TokenKind::Identifier)) {
        const Token &tok = advance();

        return std::make_unique<ast::VariableExpr>(tok.lexeme);
    }

    errorHere("expected a expression");
}