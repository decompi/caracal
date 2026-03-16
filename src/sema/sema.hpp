#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../ast/ast.hpp"

struct SemaAnalyzer {
    void analyze(const ast::Program &program);

private:
    std::unordered_map<std::string, const ast::FunctionDecl*> functions_;
    std::vector<std::unordered_set<std::string>> scopes_;

    void collectFunctions(const ast::Program &program);
    void analyzeFunction(const ast::FunctionDecl &fn);
    void analyzeBlock(const ast::BlockStmt &block);
    void analyzeStmt(const ast::Stmt &stmt);
    void analyzeExpr(const ast::Expr &expr);

    void pushScope();
    void popScope();

    void declareVariable(const std::string &name);
    bool isVariableDeclared(const std::string &name) const;

    [[noreturn]] void error(const std::string &message) const;
};