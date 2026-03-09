#pragma once

#include <string>
#include <vector>

#include "../ast/ast.hpp"
#include "../common/token.hpp"


struct Parser {
    explicit Parser(std::vector<Token> tokens);

    ast::Program parseProgram();

private:
    std::vector<Token> tokens_;
    std::size_t current_;

    const Token &peek() const;
    const Token &previous() const;
    bool isAtEnd() const;

    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token &advance();

    const Token &consume(TokenKind kind, const std::string &message);

    ast::FunctionDeclPtr parseFunction();
    ast::BlockStmtPtr parseBlock();
    ast::StmtPtr parseStatement();
    ast::StmtPtr parseLetStmt();
    ast::StmtPtr parseReturnStmt();

    ast::ExprPtr parseExpression();
    ast::ExprPtr parsePrimary();

    [[noreturn]] void errorHere(const std::string &message) const;
};