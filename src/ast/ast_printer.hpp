#pragma once

#include <ostream>
#include "ast.hpp"

namespace ast {
    struct AstPrinter {
        explicit AstPrinter(std::ostream &out);
        void print(const Program &program);
    private:
        std::ostream &out_;

        void indent(int level);

        void printFunctionDecl(const FunctionDecl &fn, int level);
        void printBlockStmt(const BlockStmt &block, int level);
        void printStmt(const Stmt &stmt, int level);
        void printExpr(const Expr &expr, int level);
    };  
};