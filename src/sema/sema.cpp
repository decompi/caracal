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
    functions_["sqrt"] = FunctionSymbol{
        {ast::Type::f64()},
        ast::Type::f64()
    };
    functions_["exp"] = FunctionSymbol{
        {ast::Type::f64()},
        ast::Type::f64()
    };

    for (const auto &fn : program.functions) {
        if (functions_.count(fn->name)) {
            error("duplicate function declaration '" + fn->name + "'");
        }

        validateSupportedFunctionSignature(*fn);

        FunctionSymbol symbol;
        for (const auto &param : fn->params) {
            symbol.paramTypes.push_back(param.type);
        }
        symbol.returnType = fn->returnType;
        functions_[fn->name] = symbol;
    }
}

void SemaAnalyzer::validateSupportedFunctionSignature(const ast::FunctionDecl &fn) const {
    for (const auto &param : fn.params) {
        if(param.type.isArray()) {
            error("array parameters are not supported yet");
        }
    }

    if(fn.returnType.isArray()) {
        error("array return types are not supported yet");
    }
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

    for (const auto &stmt : block.statements) {
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

    if (const auto *indexAssignStmt = dynamic_cast<const ast::IndexAssignStmt*>(&stmt)) {
        ast::Type arrayType = lookupVariable(indexAssignStmt->arrayName);
        if (!arrayType.isArray()) {
            error("cannot index non-array variable '" + indexAssignStmt->arrayName + "'");
        }

        if (*arrayType.elementType != ast::Type::i32() &&
            *arrayType.elementType != ast::Type::f64()) {
            error("only i32 and f64 arrays are supported yet");
        }


        ast::Type indexType = analyzeExpr(*indexAssignStmt->index);
        if (indexType != ast::Type::i32()) {
            error("array index must have type i32");
        }

        ast::Type valueType = analyzeExpr(*indexAssignStmt->value);
        if (valueType != *arrayType.elementType) {
            error(
                "cannot assign value of type " + ast::toString(valueType) +
                " to array element of type " + ast::toString(*arrayType.elementType)
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

    if (dynamic_cast<const ast::FloatExpr*>(&expr)) {
        return ast::Type::f64();
    }

    if (dynamic_cast<const ast::BoolExpr*>(&expr)) {
        return ast::Type::boolean();
    }

    if (const auto *varExpr = dynamic_cast<const ast::VariableExpr*>(&expr)) {
        return lookupVariable(varExpr->name);
    }

    if (const auto *arrayLiteralExpr = dynamic_cast<const ast::ArrayLiteralExpr*>(&expr)) {
        if (arrayLiteralExpr->elements.empty()) {
            error("array literals must contain at least one element");
        }

        ast::Type firstType = analyzeExpr(*arrayLiteralExpr->elements[0]);
        if (firstType != ast::Type::i32() && firstType != ast::Type::f64()) {
            error("only i32 and f64 array literals are supported yet");
        }

        for (std::size_t i = 1; i < arrayLiteralExpr->elements.size(); ++i) {
            ast::Type elementType = analyzeExpr(*arrayLiteralExpr->elements[i]);
            if (elementType != firstType) {
                error("array literal elements must all have the same type");
            }
        }

        return ast::Type::array(firstType, arrayLiteralExpr->elements.size());
    }

    if (const auto *indexExpr = dynamic_cast<const ast::IndexExpr*>(&expr)) {
        ast::Type baseType = analyzeExpr(*indexExpr->base);
        if (!baseType.isArray()) {
            error("cannot index a non-array expression");
        }

        ast::Type indexType = analyzeExpr(*indexExpr->index);
        if (indexType != ast::Type::i32()) {
            error("array index must have type i32");
        }

        return *baseType.elementType;
    }

    if (const auto *unaryExpr = dynamic_cast<const ast::UnaryExpr*>(&expr)) {
        ast::Type operandType = analyzeExpr(*unaryExpr->operand);

        if (unaryExpr->op == "-") {
            if (operandType == ast::Type::i32()) {
                return ast::Type::i32();
            }
            if (operandType == ast::Type::f64()) {
                return ast::Type::f64();
            }
            error("unary '-' requires an i32 or f64 operand");
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

        if (op == "+" || op == "-" || op == "*" || op == "/") {
            if (leftType == ast::Type::i32() && rightType == ast::Type::i32()) {
                return ast::Type::i32();
            }

            if (leftType == ast::Type::f64() && rightType == ast::Type::f64()) {
                return ast::Type::f64();
            }

            error("arithmetic operators require matching i32 or f64 operands");
        }

        if (op == "%") {
            if (leftType != ast::Type::i32() || rightType != ast::Type::i32()) {
                error("operator '%' requires i32 operands");
            }
            return ast::Type::i32();
        }

        if (op == "<" || op == "<=" || op == ">" || op == ">=") {
            if (leftType == ast::Type::i32() && rightType == ast::Type::i32()) {
                return ast::Type::boolean();
            }

            if (leftType == ast::Type::f64() && rightType == ast::Type::f64()) {
                return ast::Type::boolean();
            }

            error("comparison operators require matching i32 or f64 operands");
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
        if (callExpr->callee == "print") {
            if (callExpr->arguments.size() != 1) {
                error("print expects exactly 1 argument");
            }

            ast::Type argType = analyzeExpr(*callExpr->arguments[0]);
            if (argType != ast::Type::i32() && argType != ast::Type::f64()) {
                error("print expects an i32 or f64 argument");
            }

            return ast::Type::voidType();
        }

        if (callExpr->callee == "sqrt" || callExpr->callee == "exp") {
            if (callExpr->arguments.size() != 1) {
                error(callExpr->callee + " expects exactly 1 argument");
            }

            ast::Type argType = analyzeExpr(*callExpr->arguments[0]);
            if (argType != ast::Type::f64()) {
                error(callExpr->callee + " expects an f64 argument");
            }

            return ast::Type::f64();
        }

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
    if (!scopes_.empty()) {
        scopes_.pop_back();
    }
}

void SemaAnalyzer::declareVariable(const std::string &name, ast::Type type) {
    if (scopes_.empty()) {
        error("internal sema error: no active scope for declaration '" + name + "'");
    }

    auto &scope = scopes_.back();
    if (scope.count(name)) {
        error("duplicate variable declaration '" + name + "'");
    }

    scope[name] = type;
}

ast::Type SemaAnalyzer::lookupVariable(const std::string &name) const {
    for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }

    error("use of undeclared variable '" + name + "'");
}

[[noreturn]] void SemaAnalyzer::error(const std::string &message) const {
    throw std::runtime_error("semantic error: " + message);
}
