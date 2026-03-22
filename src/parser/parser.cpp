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

std::vector<ast::Parameter> Parser::parseParameterList() {
    std::vector<ast::Parameter> params;

    if(check(TokenKind::RParen)) {
        return params;
    }

    do {
        const Token &nameTok = consume(TokenKind::Identifier, "expected parameter name");
        consume(TokenKind::Colon, "expected ':' after parameter name");
        consume(TokenKind::I32, "expected parameter type i32");

        params.emplace_back(nameTok.lexeme, "i32");
    } while(match(TokenKind::Comma));

    return params;
}

std::vector<ast::ExprPtr> Parser::parseArgumentList() {
    std::vector<ast::ExprPtr> args;

    if(check(TokenKind::RParen)) {
        return args;
    }

    do {
        args.push_back(parseExpression());
    } while(match(TokenKind::Comma));

    return args;
}

ast::ExprPtr Parser::parsePostfix() {
    auto expr = parsePrimary();

    while(check(TokenKind::LParen)) {
        advance(); // so it consumes '('

        auto args = parseArgumentList();
        consume(TokenKind::RParen, "expected ')' after argument list");

        auto *varExpr = dynamic_cast<ast::VariableExpr*>(expr.get());
        if(!varExpr) {
            errorHere("can only call a function by identifier");
        }

        std::string callee = varExpr->name;
        expr = std::make_unique<ast::CallExpr>(callee, std::move(args));
    }

    return expr;
}

ast::FunctionDeclPtr Parser::parseFunction() {
    consume(TokenKind::Fn, "expected fn");

    const Token &nameTok = consume(TokenKind::Identifier, "expected function name");
    std::string name = nameTok.lexeme;

    consume(TokenKind::LParen, "expected ( after function name");
    auto params = parseParameterList();
    consume(TokenKind::RParen, "expected ) after function params");

    consume(TokenKind::Arrow, "expected -> after function param list");
    // keep the return type at i32 only for testing
    // change later with a system that can handle diff types
    // when more types get added
    consume(TokenKind::I32, "expected return type i32");

    auto body = parseBlock();

    return std::make_unique<ast::FunctionDecl>(
        name,
        std::move(params),
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

    consume(TokenKind::RBrace, "expected } to end the block");

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
    
    if(check(TokenKind::If)) {
        return parseIfStmt();
    }
    if(check(TokenKind::While)) {
        return parseWhileStmt();
    }

    // default fallback to parseExpressionStmt?
    return parseAssignmentOrExprStmt();

    //errorHere("expected statement");
}

ast::StmtPtr Parser::parseAssignmentOrExprStmt() {
    if(check(TokenKind::Identifier) && current_ + 1 < tokens_.size() && tokens_[current_ + 1].kind == TokenKind::Equal) {
        const Token &nameTok = advance();
        std::string name = nameTok.lexeme;

        consume(TokenKind::Equal, "expected '=' in assignment");

        auto value = parseExpression();

        consume(TokenKind::Semicolon, "expected ';' after assignment");

        return std::make_unique<ast::AssignStmt>(name, std::move(value));
    }

    auto expr = parseExpression();
    consume(TokenKind::Semicolon, "expected ';' after expression statement");
    return std::make_unique<ast::ExprStmt>(std::move(expr));
}

ast::StmtPtr Parser::parseIfStmt() {
    consume(TokenKind::If, "expected 'if'");
    consume(TokenKind::LParen, "expected '(' after 'if'");

    auto condition = parseExpression();
    consume(TokenKind::RParen, "expected ')' after if condition");

    auto thenBranch = parseBlock();
    ast::BlockStmtPtr elseBranch = nullptr;

    if(check(TokenKind::Else)) {
        advance();
        elseBranch = parseBlock();
    }

    return std::make_unique<ast::IfStmt>(std::move(condition), std::move(thenBranch), std::move(elseBranch));
}   

ast::StmtPtr Parser::parseWhileStmt() {
    consume(TokenKind::While, "expected 'while'");
    consume(TokenKind::LParen, "expected '(' after 'while'");

    auto condition = parseExpression();

    consume(TokenKind::RParen, "expected ')' after while condition");

    auto body = parseBlock();

    return std::make_unique<ast::WhileStmt>(std::move(condition), std::move(body));
}

ast::StmtPtr Parser::parseLetStmt() {
    consume(TokenKind::Let, "expected let");

    const Token &nameTok = consume(TokenKind::Identifier, "expected a variable name");
    std::string varName = nameTok.lexeme;

    consume(TokenKind::Colon, "expected a ':' after the variable name");
    
    // change this when more types are added
    consume(TokenKind::I32, "expected type i32");

    consume(TokenKind::Equal, "expected a = after the variable name");

    auto initializer = parseExpression();

    consume(TokenKind::Semicolon, "expected ; after the let statement");

    return std::make_unique<ast::LetStmt>(
        varName,
        "i32",
        std::move(initializer)
    );
}

ast::StmtPtr Parser::parseReturnStmt() {
    consume(TokenKind::Return, "expected return");

    auto value = parseExpression();

    consume(TokenKind::Semicolon, "expected ; after return statement");

    return std::make_unique<ast::ReturnStmt>(std::move(value));
}

ast::ExprPtr Parser::parseExpression() {
    return parseEquality();
}

ast::ExprPtr Parser::parseEquality() {
    auto expr = parseComparison();

    while(check(TokenKind::EqualEqual) || check(TokenKind::BangEqual)) {
        Token op = advance();
        auto right = parseComparison();
        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    } 
    
    return expr;
}

ast::ExprPtr Parser::parseComparison() {
    auto expr = parseAdditive();

    while(check(TokenKind::Less) || check(TokenKind::LessEqual) || check(TokenKind::Greater) || check(TokenKind::GreaterEqual)) {
        Token op = advance();

        auto right = parseAdditive();

        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

ast::ExprPtr Parser::parseAdditive() {
    auto expr = parseMultiplicative();

    while(check(TokenKind::Plus) || check(TokenKind::Minus)) {
        Token op = advance();
        auto right = parseMultiplicative();
        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

ast::ExprPtr Parser::parseMultiplicative() {
    auto expr = parseUnary();

    while(check(TokenKind::Star) || check(TokenKind::Slash) || check(TokenKind::Percent)) {
        Token op = advance();
        auto right = parseUnary();
        expr = std::make_unique<ast::BinaryExpr>(std::move(expr), op.lexeme, std::move(right));
    }

    return expr;
}

ast::ExprPtr Parser::parseUnary() {
    if(check(TokenKind::Minus)) {
        Token op = advance();
        auto operand = parseUnary();
        return std::make_unique<ast::UnaryExpr>(op.lexeme, std::move(operand));
    }

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

    if(check(TokenKind::LParen)) {
        advance();
        auto expr = parseExpression();
        consume(TokenKind::RParen, "expected ')' after expression");
        return expr;
    }

    errorHere("expected an expression");
}