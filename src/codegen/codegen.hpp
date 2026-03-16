#pragma once

#include <ostream>
#include "../ast/ast.hpp"

struct CodeGenerator {
    explicit CodeGenerator(std::ostream &out);
    void generate(const ast::Program &program);
private:
    std::ostream &out_;

    void generateFunction(const ast::FunctionDecl &fn);
    void generateStmt(const ast::Stmt &stmt);
    void generateExpr(const ast::Expr &expr);

    [[noreturn]] void error(const std::string &message) const;
};