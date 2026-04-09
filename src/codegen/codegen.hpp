#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../ast/ast.hpp"

struct LocalInfo {
    int offset;
    ast::Type type;
};

struct CodeGenerator {
    explicit CodeGenerator(std::ostream &out);
    void generate(const ast::Program &program);

private:
    static constexpr int FRAME_SIZE = 512;
    static constexpr int LOCAL_SLOT_SIZE = 16;
    static constexpr int TEMP_BASE_OFFSET = 256;

    std::ostream &out_;

    std::vector<std::unordered_map<std::string, LocalInfo>> localScopes_;
    std::unordered_map<std::string, ast::Type> functionReturnTypes_;
    std::unordered_map<std::string, std::vector<ast::Type>> functionParamTypes_;
    std::vector<std::pair<std::string, double>> floatConstants_;

    int nextLocalOffset_;
    int tempDepth_;
    int labelCounter_;
    std::string currentReturnLabel_;
    std::string currentBoundsTrapLabel_;

    void collectFunctionSignatures(const ast::Program &program);

    void generateFunction(const ast::FunctionDecl &fn);
    void generateBlock(const ast::BlockStmt &block);
    void generateStmt(const ast::Stmt &stmt);
    void generateExpr(const ast::Expr &expr);

    void pushScope();
    void popScope();

    LocalInfo declareLocal(const std::string &name, const ast::Type &type);
    LocalInfo lookupLocal(const std::string &name) const;

    ast::Type inferExprType(const ast::Expr &expr) const;

    int currentTempOffset() const;
    std::string makeLabel(const std::string &prefix);
    std::string registerFloatConstant(double value);
    void generateArrayBoundsCheck(const LocalInfo &arrayInfo);

    [[noreturn]] void error(const std::string &message) const;
};