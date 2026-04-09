#include "codegen.hpp"

#include <iomanip>
#include <stdexcept>
#include <vector>

CodeGenerator::CodeGenerator(std::ostream &out)
    : out_(out), nextLocalOffset_(16), tempDepth_(0), labelCounter_(0) {
}

void CodeGenerator::generate(const ast::Program &program) {
    functionReturnTypes_.clear();
    functionParamTypes_.clear();
    floatConstants_.clear();

    collectFunctionSignatures(program);

    out_ << ".section .rodata\n";
    out_ << ".Lprint_i32_fmt:\n";
    out_ << "    .asciz \"%d\\n\"\n\n";
    out_ << ".Lprint_f64_fmt:\n";
    out_ << "    .asciz \"%f\\n\"\n\n";

    out_ << ".text\n";
    out_ << ".extern printf\n";
    out_ << ".extern abort\n\n";

    for (const auto &fn : program.functions) {
        generateFunction(*fn);
        out_ << "\n";
    }

    if (!floatConstants_.empty()) {
        out_ << ".section .rodata\n";
        out_ << ".align 3\n";

        for (const auto &entry : floatConstants_) {
            out_ << entry.first << ":\n";
            out_ << "    .double " << std::setprecision(17) << entry.second << "\n";
        }
    }
}

void CodeGenerator::collectFunctionSignatures(const ast::Program &program) {
    functionParamTypes_["print"] = {ast::Type::i32()};
    functionReturnTypes_["print"] = ast::Type::voidType();

    for (const auto &fn : program.functions) {
        std::vector<ast::Type> paramTypes;
        for (const auto &param : fn->params) {
            paramTypes.push_back(param.type);
        }

        functionParamTypes_[fn->name] = std::move(paramTypes);
        functionReturnTypes_[fn->name] = fn->returnType;
    }
}

void CodeGenerator::pushScope() {
    localScopes_.push_back({});
}

void CodeGenerator::popScope() {
    if (!localScopes_.empty()) {
        localScopes_.pop_back();
    }
}

LocalInfo CodeGenerator::declareLocal(const std::string &name, const ast::Type &type) {
    if (localScopes_.empty()) {
        error("internal codegen error: no active scope for local '" + name + "'");
    }

    auto &scope = localScopes_.back();
    if (scope.count(name)) {
        error("internal codegen error: duplicate local in current scope '" + name + "'");
    }

    int slotCount = 1;
    if (type.isArray()) {
        if (*type.elementType != ast::Type::i32()) {
            error("only i32 arrays are supported in codegen");
        }
        slotCount = static_cast<int>(type.arrayLength);
    }

    int offset = nextLocalOffset_;
    nextLocalOffset_ += slotCount * LOCAL_SLOT_SIZE;

    if (nextLocalOffset_ >= TEMP_BASE_OFFSET) {
        error("stack frame exhausted; increase FRAME_SIZE/TEMP_BASE_OFFSET");
    }

    LocalInfo info{offset, type};
    scope[name] = info;
    return info;
}

LocalInfo CodeGenerator::lookupLocal(const std::string &name) const {
    for (auto it = localScopes_.rbegin(); it != localScopes_.rend(); ++it) {
        auto found = it->find(name);
        if (found != it->end()) {
            return found->second;
        }
    }

    error("unknown local variable '" + name + "'");
}

ast::Type CodeGenerator::inferExprType(const ast::Expr &expr) const {
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
        return lookupLocal(varExpr->name).type;
    }

    if (const auto *arrayLiteralExpr = dynamic_cast<const ast::ArrayLiteralExpr*>(&expr)) {
        return ast::Type::array(ast::Type::i32(), arrayLiteralExpr->elements.size());
    }

    if (const auto *indexExpr = dynamic_cast<const ast::IndexExpr*>(&expr)) {
        ast::Type baseType = inferExprType(*indexExpr->base);
        if (!baseType.isArray()) {
            error("internal codegen error: indexing non-array expression");
        }
        return *baseType.elementType;
    }

    if (const auto *unaryExpr = dynamic_cast<const ast::UnaryExpr*>(&expr)) {
        ast::Type operandType = inferExprType(*unaryExpr->operand);

        if (unaryExpr->op == "-") {
            return operandType;
        }

        if (unaryExpr->op == "!") {
            return ast::Type::boolean();
        }

        error("internal codegen error: unknown unary operator '" + unaryExpr->op + "'");
    }

    if (const auto *binaryExpr = dynamic_cast<const ast::BinaryExpr*>(&expr)) {
        ast::Type leftType = inferExprType(*binaryExpr->left);
        const std::string &op = binaryExpr->op;

        if (op == "+" || op == "-" || op == "*" || op == "/" || op == "%") {
            return leftType;
        }

        if (op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=") {
            return ast::Type::boolean();
        }

        error("internal codegen error: unknown binary operator '" + op + "'");
    }

    if (const auto *callExpr = dynamic_cast<const ast::CallExpr*>(&expr)) {
        auto it = functionReturnTypes_.find(callExpr->callee);
        if (it == functionReturnTypes_.end()) {
            error("internal codegen error: unknown function '" + callExpr->callee + "'");
        }
        return it->second;
    }

    error("internal codegen error: unsupported expression type inference");
}

int CodeGenerator::currentTempOffset() const {
    return TEMP_BASE_OFFSET + tempDepth_ * LOCAL_SLOT_SIZE;
}

std::string CodeGenerator::makeLabel(const std::string &prefix) {
    return ".L" + prefix + "_" + std::to_string(labelCounter_++);
}

std::string CodeGenerator::registerFloatConstant(double value) {
    std::string label = makeLabel("f64_const");
    floatConstants_.push_back({label, value});
    return label;
}

void CodeGenerator::generateArrayBoundsCheck(const LocalInfo &arrayInfo) {
    if (!arrayInfo.type.isArray()) {
        error("internal codegen error: bounds check on non-array");
    }

    out_ << "    cmp w0, #0\n";
    out_ << "    blt " << currentBoundsTrapLabel_ << "\n";
    out_ << "    cmp w0, #" << arrayInfo.type.arrayLength << "\n";
    out_ << "    bge " << currentBoundsTrapLabel_ << "\n";
}

void CodeGenerator::generateFunction(const ast::FunctionDecl &fn) {
    localScopes_.clear();
    nextLocalOffset_ = 16;
    tempDepth_ = 0;
    currentReturnLabel_ = makeLabel("return");
    currentBoundsTrapLabel_ = makeLabel("bounds_trap");

    out_ << ".global " << fn.name << "\n";
    out_ << ".align 2\n";
    out_ << fn.name << ":\n";

    out_ << "    stp x29, x30, [sp, -16]!\n";
    out_ << "    mov x29, sp\n";
    out_ << "    sub sp, sp, #" << FRAME_SIZE << "\n";
    out_ << "    mov x28, sp\n";

    pushScope();

    if (fn.params.size() > 8) {
        error("more than 8 function parameters not supported yet");
    }

    for (std::size_t i = 0; i < fn.params.size(); ++i) {
        if (fn.params[i].type.isArray()) {
            error("array parameters are not supported in codegen");
        }

        LocalInfo info = declareLocal(fn.params[i].name, fn.params[i].type);
        if (fn.params[i].type == ast::Type::f64()) {
            out_ << "    str d" << i << ", [x28, #" << info.offset << "]\n";
        } else {
            out_ << "    str w" << i << ", [x28, #" << info.offset << "]\n";
        }
    }

    generateBlock(*fn.body);

    popScope();

    out_ << currentBoundsTrapLabel_ << ":\n";
    out_ << "    bl abort\n";

    out_ << currentReturnLabel_ << ":\n";
    out_ << "    add sp, sp, #" << FRAME_SIZE << "\n";
    out_ << "    ldp x29, x30, [sp], 16\n";
    out_ << "    ret\n";
}

void CodeGenerator::generateBlock(const ast::BlockStmt &block) {
    pushScope();

    for (const auto &stmt : block.statements) {
        generateStmt(*stmt);
    }

    popScope();
}

void CodeGenerator::generateStmt(const ast::Stmt &stmt) {
    if (const auto *letStmt = dynamic_cast<const ast::LetStmt*>(&stmt)) {
        LocalInfo info = declareLocal(letStmt->name, letStmt->type);

        if (letStmt->type.isArray()) {
            const auto *arrayLiteral = dynamic_cast<const ast::ArrayLiteralExpr*>(letStmt->initializer.get());
            if (!arrayLiteral) {
                error("array local must be initialized with an array literal");
            }

            for (std::size_t i = 0; i < arrayLiteral->elements.size(); ++i) {
                generateExpr(*arrayLiteral->elements[i]);
                out_ << "    str w0, [x28, #" << (info.offset + static_cast<int>(i) * LOCAL_SLOT_SIZE) << "]\n";
            }
            return;
        }

        generateExpr(*letStmt->initializer);
        if (letStmt->type == ast::Type::f64()) {
            out_ << "    str d0, [x28, #" << info.offset << "]\n";
        } else {
            out_ << "    str w0, [x28, #" << info.offset << "]\n";
        }
        return;
    }

    if (const auto *assignStmt = dynamic_cast<const ast::AssignStmt*>(&stmt)) {
        LocalInfo info = lookupLocal(assignStmt->name);
        generateExpr(*assignStmt->value);

        if (info.type == ast::Type::f64()) {
            out_ << "    str d0, [x28, #" << info.offset << "]\n";
        } else {
            out_ << "    str w0, [x28, #" << info.offset << "]\n";
        }
        return;
    }

    if (const auto *indexAssignStmt = dynamic_cast<const ast::IndexAssignStmt*>(&stmt)) {
        LocalInfo arrayInfo = lookupLocal(indexAssignStmt->arrayName);
        if (!arrayInfo.type.isArray()) {
            error("internal codegen error: indexed assignment on non-array");
        }

        int indexTemp = currentTempOffset();
        generateExpr(*indexAssignStmt->index);
        generateArrayBoundsCheck(arrayInfo);
        out_ << "    str w0, [x28, #" << indexTemp << "]\n";
        tempDepth_++;

        generateExpr(*indexAssignStmt->value);

        tempDepth_--;
        out_ << "    ldr w1, [x28, #" << indexTemp << "]\n";
        out_ << "    mov w2, #" << LOCAL_SLOT_SIZE << "\n";
        out_ << "    mul w1, w1, w2\n";
        out_ << "    add x1, x28, #" << arrayInfo.offset << "\n";
        out_ << "    add x1, x1, w1, sxtw\n";
        out_ << "    str w0, [x1]\n";
        return;
    }

    if (const auto *exprStmt = dynamic_cast<const ast::ExprStmt*>(&stmt)) {
        generateExpr(*exprStmt->expr);
        return;
    }

    if (const auto *returnStmt = dynamic_cast<const ast::ReturnStmt*>(&stmt)) {
        generateExpr(*returnStmt->value);
        out_ << "    b " << currentReturnLabel_ << "\n";
        return;
    }

    if (const auto *ifStmt = dynamic_cast<const ast::IfStmt*>(&stmt)) {
        std::string elseLabel = makeLabel("if_else");
        std::string endLabel = makeLabel("if_end");

        generateExpr(*ifStmt->condition);
        out_ << "    cmp w0, #0\n";

        if (ifStmt->elseBranch) {
            out_ << "    beq " << elseLabel << "\n";
            generateBlock(*ifStmt->thenBranch);
            out_ << "    b " << endLabel << "\n";
            out_ << elseLabel << ":\n";
            generateBlock(*ifStmt->elseBranch);
            out_ << endLabel << ":\n";
        } else {
            out_ << "    beq " << endLabel << "\n";
            generateBlock(*ifStmt->thenBranch);
            out_ << endLabel << ":\n";
        }
        return;
    }

    if (const auto *whileStmt = dynamic_cast<const ast::WhileStmt*>(&stmt)) {
        std::string condLabel = makeLabel("while_cond");
        std::string endLabel = makeLabel("while_end");

        out_ << condLabel << ":\n";
        generateExpr(*whileStmt->condition);
        out_ << "    cmp w0, #0\n";
        out_ << "    beq " << endLabel << "\n";
        generateBlock(*whileStmt->body);
        out_ << "    b " << condLabel << "\n";
        out_ << endLabel << ":\n";
        return;
    }

    if (const auto *blockStmt = dynamic_cast<const ast::BlockStmt*>(&stmt)) {
        generateBlock(*blockStmt);
        return;
    }

    error("unsupported statement in codegen");
}

void CodeGenerator::generateExpr(const ast::Expr &expr) {
    if (const auto *intExpr = dynamic_cast<const ast::IntegerExpr*>(&expr)) {
        out_ << "    mov w0, #" << intExpr->value << "\n";
        return;
    }

    if (const auto *floatExpr = dynamic_cast<const ast::FloatExpr*>(&expr)) {
        std::string label = registerFloatConstant(floatExpr->value);
        out_ << "    adrp x9, " << label << "\n";
        out_ << "    add x9, x9, :lo12:" << label << "\n";
        out_ << "    ldr d0, [x9]\n";
        return;
    }

    if (const auto *varExpr = dynamic_cast<const ast::VariableExpr*>(&expr)) {
        LocalInfo info = lookupLocal(varExpr->name);
        if (info.type == ast::Type::f64()) {
            out_ << "    ldr d0, [x28, #" << info.offset << "]\n";
        } else {
            out_ << "    ldr w0, [x28, #" << info.offset << "]\n";
        }
        return;
    }

    if (const auto *boolExpr = dynamic_cast<const ast::BoolExpr*>(&expr)) {
        out_ << "    mov w0, #" << (boolExpr->value ? 1 : 0) << "\n";
        return;
    }

    if (const auto *indexExpr = dynamic_cast<const ast::IndexExpr*>(&expr)) {
        const auto *baseVar = dynamic_cast<const ast::VariableExpr*>(indexExpr->base.get());
        if (!baseVar) {
            error("only variable-based array indexing is supported in codegen");
        }

        LocalInfo arrayInfo = lookupLocal(baseVar->name);
        if (!arrayInfo.type.isArray()) {
            error("internal codegen error: indexing non-array");
        }

        generateExpr(*indexExpr->index);
        generateArrayBoundsCheck(arrayInfo);
        out_ << "    mov w1, #" << LOCAL_SLOT_SIZE << "\n";
        out_ << "    mul w0, w0, w1\n";
        out_ << "    add x1, x28, #" << arrayInfo.offset << "\n";
        out_ << "    add x1, x1, w0, sxtw\n";
        out_ << "    ldr w0, [x1]\n";
        return;
    }

    if (const auto *unaryExpr = dynamic_cast<const ast::UnaryExpr*>(&expr)) {
        ast::Type operandType = inferExprType(*unaryExpr->operand);
        generateExpr(*unaryExpr->operand);

        if (unaryExpr->op == "-") {
            if (operandType == ast::Type::f64()) {
                out_ << "    fneg d0, d0\n";
            } else {
                out_ << "    neg w0, w0\n";
            }
            return;
        }

        if (unaryExpr->op == "!") {
            out_ << "    cmp w0, #0\n";
            out_ << "    cset w0, eq\n";
            return;
        }

        error("unsupported unary operator '" + unaryExpr->op + "'");
    }

    if (const auto *binaryExpr = dynamic_cast<const ast::BinaryExpr*>(&expr)) {
        ast::Type leftType = inferExprType(*binaryExpr->left);
        int tempOffset = currentTempOffset();

        if (leftType == ast::Type::f64()) {
            generateExpr(*binaryExpr->left);
            out_ << "    str d0, [x28, #" << tempOffset << "]\n";
            tempDepth_++;

            generateExpr(*binaryExpr->right);

            tempDepth_--;
            out_ << "    ldr d1, [x28, #" << tempOffset << "]\n";

            const std::string &op = binaryExpr->op;

            if (op == "+") {
                out_ << "    fadd d0, d1, d0\n";
                return;
            }

            if (op == "-") {
                out_ << "    fsub d0, d1, d0\n";
                return;
            }

            if (op == "*") {
                out_ << "    fmul d0, d1, d0\n";
                return;
            }

            if (op == "/") {
                out_ << "    fdiv d0, d1, d0\n";
                return;
            }

            if (op == "==") {
                out_ << "    fcmp d1, d0\n";
                out_ << "    cset w0, eq\n";
                return;
            }

            if (op == "!=") {
                out_ << "    fcmp d1, d0\n";
                out_ << "    cset w0, ne\n";
                return;
            }

            if (op == "<") {
                out_ << "    fcmp d1, d0\n";
                out_ << "    cset w0, lt\n";
                return;
            }

            if (op == "<=") {
                out_ << "    fcmp d1, d0\n";
                out_ << "    cset w0, le\n";
                return;
            }

            if (op == ">") {
                out_ << "    fcmp d1, d0\n";
                out_ << "    cset w0, gt\n";
                return;
            }

            if (op == ">=") {
                out_ << "    fcmp d1, d0\n";
                out_ << "    cset w0, ge\n";
                return;
            }

            error("unsupported f64 binary operator '" + op + "'");
        }

        generateExpr(*binaryExpr->left);
        out_ << "    str w0, [x28, #" << tempOffset << "]\n";
        tempDepth_++;

        generateExpr(*binaryExpr->right);

        tempDepth_--;
        out_ << "    ldr w1, [x28, #" << tempOffset << "]\n";

        const std::string &op = binaryExpr->op;

        if (op == "+") {
            out_ << "    add w0, w1, w0\n";
            return;
        }

        if (op == "-") {
            out_ << "    sub w0, w1, w0\n";
            return;
        }

        if (op == "*") {
            out_ << "    mul w0, w1, w0\n";
            return;
        }

        if (op == "/") {
            out_ << "    sdiv w0, w1, w0\n";
            return;
        }

        if (op == "%") {
            out_ << "    sdiv w2, w1, w0\n";
            out_ << "    msub w0, w2, w0, w1\n";
            return;
        }

        if (op == "==") {
            out_ << "    cmp w1, w0\n";
            out_ << "    cset w0, eq\n";
            return;
        }

        if (op == "!=") {
            out_ << "    cmp w1, w0\n";
            out_ << "    cset w0, ne\n";
            return;
        }

        if (op == "<") {
            out_ << "    cmp w1, w0\n";
            out_ << "    cset w0, lt\n";
            return;
        }

        if (op == "<=") {
            out_ << "    cmp w1, w0\n";
            out_ << "    cset w0, le\n";
            return;
        }

        if (op == ">") {
            out_ << "    cmp w1, w0\n";
            out_ << "    cset w0, gt\n";
            return;
        }

        if (op == ">=") {
            out_ << "    cmp w1, w0\n";
            out_ << "    cset w0, ge\n";
            return;
        }

        error("unsupported binary operator '" + op + "'");
    }

    if (const auto *callExpr = dynamic_cast<const ast::CallExpr*>(&expr)) {
        if (callExpr->callee == "print") {
            if (callExpr->arguments.size() != 1) {
                error("print expects exactly 1 argument");
            }

            ast::Type argType = inferExprType(*callExpr->arguments[0]);
            generateExpr(*callExpr->arguments[0]);

            if (argType == ast::Type::i32()) {
                out_ << "    mov w1, w0\n";
                out_ << "    adrp x0, .Lprint_i32_fmt\n";
                out_ << "    add x0, x0, :lo12:.Lprint_i32_fmt\n";
                out_ << "    bl printf\n";
                out_ << "    mov w0, #0\n";
                return;
            }

            if (argType == ast::Type::f64()) {
                out_ << "    adrp x0, .Lprint_f64_fmt\n";
                out_ << "    add x0, x0, :lo12:.Lprint_f64_fmt\n";
                out_ << "    bl printf\n";
                out_ << "    mov w0, #0\n";
                return;
            }

            error("print only supports i32 and f64 in codegen");
        }

        auto it = functionParamTypes_.find(callExpr->callee);
        if (it == functionParamTypes_.end()) {
            error("unknown function '" + callExpr->callee + "'");
        }

        const auto &paramTypes = it->second;

        if (callExpr->arguments.size() > 8) {
            error("more than 8 call arguments not supported yet");
        }

        std::vector<std::pair<int, ast::Type>> argTemps;
        argTemps.reserve(callExpr->arguments.size());

        int savedTempDepth = tempDepth_;

        for (const auto &arg : callExpr->arguments) {
            ast::Type argType = inferExprType(*arg);
            int argOffset = currentTempOffset();

            generateExpr(*arg);

            if (argType == ast::Type::f64()) {
                out_ << "    str d0, [x28, #" << argOffset << "]\n";
            } else {
                out_ << "    str w0, [x28, #" << argOffset << "]\n";
            }

            argTemps.push_back({argOffset, argType});
            tempDepth_++;
        }

        for (std::size_t i = 0; i < argTemps.size(); ++i) {
            if (paramTypes[i] == ast::Type::f64()) {
                out_ << "    ldr d" << i << ", [x28, #" << argTemps[i].first << "]\n";
            } else {
                out_ << "    ldr w" << i << ", [x28, #" << argTemps[i].first << "]\n";
            }
        }

        tempDepth_ = savedTempDepth;
        out_ << "    bl " << callExpr->callee << "\n";
        return;
    }

    error("unsupported expression in codegen");
}

[[noreturn]] void CodeGenerator::error(const std::string &message) const {
    throw std::runtime_error("codegen error: " + message);
}