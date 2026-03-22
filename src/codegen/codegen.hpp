#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "../ast/ast.hpp"

struct CodeGenerator {
    explicit CodeGenerator(std::ostream &out);
    void generate(const ast::Program &program);
private:
    static constexpr int FRAME_SIZE = 512;
    static constexpr int LOCAL_SLOT_SIZE = 16;
    static constexpr int TEMP_BASE_OFFSET = 256;

    std::ostream &out_;

    std::vector<std::unordered_map<std::string, int>> localScopes_;
    int nextLocalOffset_;
    int tempDepth_;
    int labelCounter_;
    std::string currentReturnLabel_;

    void generateFunction(const ast::FunctionDecl &fn);
    void generateBlock(const ast::BlockStmt &block);
    void generateStmt(const ast::Stmt &stmt);
    void generateExpr(const ast::Expr &expr);

    void pushScope();
    void popScope();

    int declareLocal(const std::string &name);
    int lookupLocal(const std::string &name) const;

    int currentTempOffset() const;
    std::string makeLabel(const std::string &prefix);

    [[noreturn]] void error(const std::string &message) const;
};