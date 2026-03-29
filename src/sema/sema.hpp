#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "../ast/ast.hpp"

struct FunctionSymbol {
    std::vector<ast::Type> paramTypes;
    ast::Type returnType;
};

struct SemaAnalyzer {
    void analyze(const ast::Program &program);

private:
    std::unordered_map<std::string, FunctionSymbol> functions_;
    std::vector<std::unordered_map<std::string, ast::Type>> scopes_;
    ast::Type currentFunctionReturnType_ = ast::Type::error();

    void collectFunctions(const ast::Program &program);
    void analyzeFunction(const ast::FunctionDecl &fn);
    void analyzeBlock(const ast::BlockStmt &block);
    void analyzeStmt(const ast::Stmt &stmt);
    ast::Type analyzeExpr(const ast::Expr &expr);

    void pushScope();
    void popScope();

    void declareVariable(const std::string &name, ast::Type type);
    ast::Type lookupVariable(const std::string &name) const;
    void validateMainSignature() const;

    [[noreturn]] void error(const std::string &message) const;
};