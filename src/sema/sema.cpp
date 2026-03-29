#include "sema.hpp"

#include <stdexcept>

void SemaAnalyzer::analyze(const ast::Program &program) {
    functions_.clear();
    scopes_.clear();

    collectFunctions(program);
    validateMainSignature();

    for(const auto &fn : program.functions) {
        analyzeFunction(*fn);
    }
}

void SemaAnalyzer::collectFunctions(const ast::Program &program) {
    // built in print
    functions_["print"] = FunctionSymbol{
        {ast::Type::i32()},
        ast::Type::voidType()
    };

    for (const auto &fn : program.functions) {
        if (functions_.count(fn->name)) {
            error("duplicate function declaration '" + fn->name + "'");
        }

        FunctionSymbol symbol;
        for (const auto &param : fn->params) {
            symbol.paramTypes.push_back(param.type);
        }
        symbol.returnType = fn->returnType;
        functions_[fn->name] = symbol;
    }
}

void SemaAnalyzer::analyzeFunction(const ast::FunctionDecl &fn) {
    ast::Type previousReturnType = currentFunctionReturnType_;
    currentFunctionReturnType_ = fn.returnType;

    pushScope();
    for (const auto &param : fn.params) {
        declareVariable(param.name, param.type);
    }

    analyzeBlock(*fn.body);

    popScope();
    currentFunctionReturnType_ = previousReturnType;
}

void SemaAnalyzer::analyzeBlock(const ast::BlockStmt &block) {
    pushScope();

    for(const auto & stmt: block.statements) {
        analyzeStmt(*stmt);
    }

    popScope();
}

void SemaAnalyzer::analyzeStmt(const ast::Stmt &stmt) {
    if (const auto *letStmt = dynamic_cast<const ast::LetStmt*>(&stmt)) {
        ast::Type initializerType = analyzeExpr(*letStmt->initializer);
        if (initializerType != letStmt->type) {
            error(
                "cannot initialize variable '" + letStmt->name + "' of type " +
                ast::toString(letStmt->type) + " with value of type " +
                ast::toString(initializerType)
            );
        }

        declareVariable(letStmt->name, letStmt->type);
        return;
    }

    if (const auto *assignStmt = dynamic_cast<const ast::AssignStmt*>(&stmt)) {
        ast::Type variableType = lookupVariable(assignStmt->name);
        ast::Type valueType = analyzeExpr(*assignStmt->value);

        if (variableType != valueType) {
            error(
                "cannot assign value of type " + ast::toString(valueType) +
                " to variable '" + assignStmt->name + "' of type " +
                ast::toString(variableType)
            );
        }
        return;
    }

    if (const auto *exprStmt = dynamic_cast<const ast::ExprStmt*>(&stmt)) {
        analyzeExpr(*exprStmt->expr);
        return;
    }

    if (const auto *returnStmt = dynamic_cast<const ast::ReturnStmt*>(&stmt)) {
        ast::Type valueType = analyzeExpr(*returnStmt->value);

        if (valueType != currentFunctionReturnType_) {
            error(
                "return type mismatch: expected " +
                ast::toString(currentFunctionReturnType_) +
                ", got " + ast::toString(valueType)
            );
        }
        return;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt*>(&stmt)) {
        ast::Type conditionType = analyzeExpr(*ifStmt->condition);
        if (conditionType != ast::Type::boolean()) {
            error("if condition must have type bool");
        }

        analyzeBlock(*ifStmt->thenBranch);
        if (ifStmt->elseBranch) {
            analyzeBlock(*ifStmt->elseBranch);
        }
        return;
    }

    if (const auto *whileStmt = dynamic_cast<const ast::WhileStmt*>(&stmt)) {
        ast::Type conditionType = analyzeExpr(*whileStmt->condition);
        if (conditionType != ast::Type::boolean()) {
            error("while condition must have type bool");
        }

        analyzeBlock(*whileStmt->body);
        return;
    }

    if (const auto *blockStmt = dynamic_cast<const ast::BlockStmt*>(&stmt)) {
        analyzeBlock(*blockStmt);
        return;
    }

    error("unknown statement in semantic analysis");
}


ast::Type SemaAnalyzer::analyzeExpr(const ast::Expr &expr) {
    if (dynamic_cast<const ast::IntegerExpr*>(&expr)) {
        return ast::Type::i32();
    }

    if (dynamic_cast<const ast::BoolExpr*>(&expr)) {
        return ast::Type::boolean();
    }

    if (const auto *varExpr = dynamic_cast<const ast::VariableExpr*>(&expr)) {
        return lookupVariable(varExpr->name);
    }

    if (const auto *unaryExpr = dynamic_cast<const ast::UnaryExpr*>(&expr)) {
        ast::Type operandType = analyzeExpr(*unaryExpr->operand);

        if (unaryExpr->op == "-") {
            if (operandType != ast::Type::i32()) {
                error("unary '-' requires an i32 operand");
            }
            return ast::Type::i32();
        }

        if (unaryExpr->op == "!") {
            if (operandType != ast::Type::boolean()) {
                error("unary '!' requires a bool operand");
            }
            return ast::Type::boolean();
        }

        error("unknown unary operator '" + unaryExpr->op + "'");
    }

    if (const auto *binaryExpr = dynamic_cast<const ast::BinaryExpr*>(&expr)) {
        ast::Type leftType = analyzeExpr(*binaryExpr->left);
        ast::Type rightType = analyzeExpr(*binaryExpr->right);
        const std::string &op = binaryExpr->op;

        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            if (leftType != ast::Type::i32() || rightType != ast::Type::i32()) {
                error("arithmetic operators require i32 operands");
            }
            return ast::Type::i32();
        }

        if (op == "<" || op == "<=" || op == ">" || op == ">=") {
            if (leftType != ast::Type::i32() || rightType != ast::Type::i32()) {
                error("comparison operators require i32 operands");
            }
            return ast::Type::boolean();
        }

        if (op == "==" || op == "!=") {
            if (leftType != rightType) {
                error("equality operators require both operands to have the same type");
            }
            return ast::Type::boolean();
        }

        error("unknown binary operator '" + op + "'");
    }

    if (const auto *callExpr = dynamic_cast<const ast::CallExpr*>(&expr)) {
        auto it = functions_.find(callExpr->callee);
        if (it == functions_.end()) {
            error("call to undeclared function '" + callExpr->callee + "'");
        }

        const FunctionSymbol &fn = it->second;

        if (callExpr->arguments.size() != fn.paramTypes.size()) {
            error(
                "function '" + callExpr->callee + "' expects " +
                std::to_string(fn.paramTypes.size()) + " argument(s), got " +
                std::to_string(callExpr->arguments.size())
            );
        }

        for (std::size_t i = 0; i < callExpr->arguments.size(); ++i) {
            ast::Type argType = analyzeExpr(*callExpr->arguments[i]);
            ast::Type expectedType = fn.paramTypes[i];

            if (argType != expectedType) {
                error(
                    "argument " + std::to_string(i + 1) + " to function '" +
                    callExpr->callee + "' has type " + ast::toString(argType) +
                    ", expected " + ast::toString(expectedType)
                );
            }
        }

        return fn.returnType;
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

void SemaAnalyzer::declareVariable(const std::string &name, ast::Type type) {
    auto &scope = scopes_.back();
    if (scope.count(name)) {
        error("duplicate variable declaration '" + name + "'");
    }
    scope[name] = type;
}

/*bool SemaAnalyzer::isVariableDeclared(const std::string &name) const {
    for(auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        if(it->count(name)) {
            return true;
        }
    }

    return false;
}*/

[[noreturn]] void SemaAnalyzer::error(const std::string &message) const {
    throw std::runtime_error("semantic error: " + message);
}

void SemaAnalyzer::validateMainSignature() const {
    auto it = functions_.find("main");
    if (it == functions_.end()) {
        error("missing required function 'main'");
    }
    if (!it->second.paramTypes.empty()) {
        error("main must not take parameters");
    }
    if (it->second.returnType != ast::Type::i32()) {
        error("main must return i32");
    }
}