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
        //consume(TokenKind::I32, "expected parameter type i32");
        ast::Type type = parseType();

        params.emplace_back(nameTok.lexeme, type);
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

    while (true) {
        if (check(TokenKind::LParen)) {
            advance();

            auto args = parseArgumentList();
            consume(TokenKind::RParen, "expected ')' after argument list");

            auto *varExpr = dynamic_cast<ast::VariableExpr*>(expr.get());
            if (!varExpr) {
                errorHere("can only call a function by identifier");
            }

            std::string callee = varExpr->name;
            expr = std::make_unique<ast::CallExpr>(callee, std::move(args));
            continue;
        }

        if (check(TokenKind::LBracket)) {
            advance();
            auto index = parseExpression();
            consume(TokenKind::RBracket, "expected ']' after index expression");

            expr = std::make_unique<ast::IndexExpr>(std::move(expr), std::move(index));
            continue;
        }

        break;
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
    //consume(TokenKind::I32, "expected return type i32");
    ast::Type returnType = parseType();

    auto body = parseBlock();

    return std::make_unique<ast::FunctionDecl>(
        name,
        std::move(params),
        returnType,
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
    if (check(TokenKind::Identifier) && current_ + 1 < tokens_.size()) {
        if (tokens_[current_ + 1].kind == TokenKind::Equal) {
            const Token &nameTok = advance();
            std::string name = nameTok.lexeme;

            consume(TokenKind::Equal, "expected '=' in assignment");
            auto value = parseExpression();
            consume(TokenKind::Semicolon, "expected ';' after assignment");

            return std::make_unique<ast::AssignStmt>(name, std::move(value));
        }

        if (tokens_[current_ + 1].kind == TokenKind::LBracket) {
            const Token &nameTok = advance();
            std::string name = nameTok.lexeme;

            consume(TokenKind::LBracket, "expected '[' after array name");
            auto index = parseExpression();
            consume(TokenKind::RBracket, "expected ']' after array index");
            consume(TokenKind::Equal, "expected '=' in indexed assignment");
            auto value = parseExpression();
            consume(TokenKind::Semicolon, "expected ';' after assignment");

            return std::make_unique<ast::IndexAssignStmt>(
                name,
                std::move(index),
                std::move(value)
            );
        }
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
    //consume(TokenKind::I32, "expected type i32");
    ast::Type varType = parseType();

    consume(TokenKind::Equal, "expected a = after the variable name");

    auto initializer = parseExpression();

    consume(TokenKind::Semicolon, "expected ; after the let statement");

    return std::make_unique<ast::LetStmt>(
        varName,
        varType,
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
    if(check(TokenKind::Minus) || check(TokenKind::Bang)) {
        Token op = advance();
        auto operand = parseUnary();
        return std::make_unique<ast::UnaryExpr>(op.lexeme, std::move(operand));
    }

    return parsePostfix();
}

ast::ExprPtr Parser::parsePrimary() {
    if (check(TokenKind::Integer)) {
        const Token &tok = advance();
        int value = std::stoi(tok.lexeme);

        return std::make_unique<ast::IntegerExpr>(value);
    }

    if(check(TokenKind::Float)) {
        const Token &tok = advance();
        double value = std::stod(tok.lexeme);

        return std::make_unique<ast::FloatExpr>(value);
    }
    
    if (match(TokenKind::True)) {
        return std::make_unique<ast::BoolExpr>(true);
    }

    if (match(TokenKind::False)) {
        return std::make_unique<ast::BoolExpr>(false);
    }

    if (check(TokenKind::Identifier)) {
        const Token &tok = advance();
        return std::make_unique<ast::VariableExpr>(tok.lexeme);
    }

    if (check(TokenKind::LParen)) {
        advance();
        auto expr = parseExpression();
        consume(TokenKind::RParen, "expected ')' after expression");
        return expr;
    }

    if (check(TokenKind::LBracket)) {
        return parseArrayLiteral();
    }

    errorHere("expected an expression");
}

ast::ExprPtr Parser::parseArrayLiteral() {
    consume(TokenKind::LBracket, "expected '[' to start array literal");

    std::vector<ast::ExprPtr> elements;

    if (!check(TokenKind::RBracket)) {
        do {
            elements.push_back(parseExpression());
        } while (match(TokenKind::Comma));
    }

    consume(TokenKind::RBracket, "expected ']' to end array literal");

    return std::make_unique<ast::ArrayLiteralExpr>(std::move(elements));
}

ast::Type Parser::parseType() {
    if (match(TokenKind::I32)) {
        return ast::Type::i32();
    }

    if(match(TokenKind::F64)) {
        return ast::Type::f64();
    }

    if (match(TokenKind::Bool)) {
        return ast::Type::boolean();
    }

    if (match(TokenKind::LBracket)) {
        ast::Type elementType = parseType();
        consume(TokenKind::Semicolon, "expected ';' in array type");

        const Token &lengthTok = consume(TokenKind::Integer, "expected array length");
        std::size_t length = static_cast<std::size_t>(std::stoul(lengthTok.lexeme));

        consume(TokenKind::RBracket, "expected ']' after array type");
        return ast::Type::array(elementType, length);
    }

    errorHere("expected type");
}