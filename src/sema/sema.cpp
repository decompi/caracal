#include "sema.hpp"

#include <stdexcept>

void SemaAnalyzer::analyze(const ast::Program &program) {
    functions_.clear();
    scopes_.clear();

    collectFunctions(program);

    for(const auto &fn : program.functions) {
        analyzeFunction(*fn);
    }
}

void SemaAnalyzer::collectFunctions(const ast::Program &program) {
    for(const auto &fn : program.functions) {
        if(functions_.count(fn->name)) {
            error("duplicate function declaration '" + fn->name + "'");
        }
        functions_[fn->name] = fn.get();
    }
}

void SemaAnalyzer::analyzeFunction(const ast::FunctionDecl &fn) {
    pushScope();

    for(const auto &param : fn.params) {
        declareVariable(param.name);
    }
    analyzeBlock(*fn.body);
    popScope();
}

void SemaAnalyzer::analyzeBlock(const ast::BlockStmt &block) {
    pushScope();

    for(const auto & stmt: block.statements) {
        analyzeStmt(*stmt);
    }

    popScope();
}

void SemaAnalyzer::analyzeStmt(const ast::Stmt &stmt) {
    if(const auto *letStmt = dynamic_cast<const ast::LetStmt*>(&stmt)) {
        if(letStmt->initializer) {
            analyzeExpr(*letStmt->initializer);
        }
        declareVariable(letStmt->name);
        return;
    }

    if(const auto *assignStmt = dynamic_cast<const ast::AssignStmt*>(&stmt)) {
        if(!isVariableDeclared(assignStmt->name)) {
            error("assignment to undeclared variable '" + assignStmt->name + "'");
        }
        analyzeExpr(*assignStmt->value);
        return;
    }

    if(const auto *exprStmt = dynamic_cast<const ast::ExprStmt*>(&stmt)) {
        analyzeExpr(*exprStmt->expr);
        return;
    }

    if(const auto *returnStmt = dynamic_cast<const ast::ReturnStmt*>(&stmt)) {
        analyzeExpr(*returnStmt->value);
    }

    if(const auto* ifStmt = dynamic_cast<const ast::IfStmt*>(&stmt)) {
        analyzeExpr(*ifStmt->condition);
        analyzeBlock(*ifStmt->thenBranch);

        if(ifStmt->elseBranch) {
            analyzeBlock(*ifStmt->elseBranch);
        }
        return;
    }

    if(const auto *whileStmt = dynamic_cast<const ast::WhileStmt*>(&stmt)) {
        analyzeExpr(*whileStmt->condition);
        analyzeBlock(*whileStmt->body);
        return;
    }

    if(const auto *blockStmt = dynamic_cast<const ast::BlockStmt*>(&stmt)) {
        analyzeBlock(*blockStmt);
        return;
    }

    error("unknown statement in semantic analysis");
}

void SemaAnalyzer::analyzeExpr(const ast::Expr& expr) {
    if(dynamic_cast<const ast::IntegerExpr*>(&expr)) {
        return;
    }

    if(const auto *varExpr = dynamic_cast<const ast::VariableExpr*>(&expr)) {
        if(!isVariableDeclared(varExpr->name)) {
            error("use of undeclared variable '" + varExpr->name + "'");
        }
        return;
    }

    if(const auto *unaryExpr = dynamic_cast<const ast::UnaryExpr*>(&expr)) {
        analyzeExpr(*unaryExpr->operand);
        return;
    }

    if(const auto *binaryExpr = dynamic_cast<const ast::BinaryExpr*>(&expr)) {
        analyzeExpr(*binaryExpr->left);
        analyzeExpr(*binaryExpr->right);
        return;
    }

    if(const auto *callExpr = dynamic_cast<const ast::CallExpr*>(&expr)) {
        for(const auto &arg : callExpr->arguments) {
            analyzeExpr(*arg);
        }
        return;
    }

    error("unknown expression in semantic analysis");
}

void SemaAnalyzer::pushScope() {
    scopes_.push_back({});
}

void SemaAnalyzer::popScope() {
    if(!scopes_.empty()) {
        scopes_.pop_back();
    }
}

void SemaAnalyzer::declareVariable(const std::string &name) {
    if(scopes_.empty()) {
        error("internal sema error: no active scope for declaration '" + name + " '");
    }

    auto &currentScope = scopes_.back();
    if(currentScope.count(name)) {
        error("duplicate variable declaration '" + name + "'");
    }

    currentScope.insert(name);
}

bool SemaAnalyzer::isVariableDeclared(const std::string &name) const {
    for(auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if(it->count(name)) {
            return true;
        }
    }

    return false;
}

[[noreturn]] void SemaAnalyzer::error(const std::string &message) const {
    throw std::runtime_error("semantic error: " + message);
}