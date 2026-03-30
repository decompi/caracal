#include "codegen.hpp"

#include <stdexcept>
#include <vector>

CodeGenerator::CodeGenerator(std::ostream &out)
    : out_(out), nextLocalOffset_(16), tempDepth_(0), labelCounter_(0) {
}

void CodeGenerator::generate(const ast::Program &program) {
    out_ << ".section .rodata\n";
    out_ << ".Lprint_fmt:\n";
    out_ << "    .asciz \"%d\\n\"\n\n";

    out_ << ".text\n";
    out_ << ".extern printf\n";
    out_ << ".extern abort\n\n";

    for (const auto &fn : program.functions) {
        generateFunction(*fn);
        out_ << "\n";
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

int CodeGenerator::currentTempOffset() const {
    return TEMP_BASE_OFFSET + tempDepth_ * LOCAL_SLOT_SIZE;
}

std::string CodeGenerator::makeLabel(const std::string &prefix) {
    return ".L" + prefix + "_" + std::to_string(labelCounter_++);
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
        LocalInfo info = declareLocal(fn.params[i].name, fn.params[i].type);
        out_ << "    str w" << i << ", [x28, #" << info.offset << "]\n";
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
        out_ << "    str w0, [x28, #" << info.offset << "]\n";
        return;
    }

    if (const auto *assignStmt = dynamic_cast<const ast::AssignStmt*>(&stmt)) {
        LocalInfo info = lookupLocal(assignStmt->name);
        generateExpr(*assignStmt->value);
        out_ << "    str w0, [x28, #" << info.offset << "]\n";
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

    if (const auto *varExpr = dynamic_cast<const ast::VariableExpr*>(&expr)) {
        LocalInfo info = lookupLocal(varExpr->name);
        out_ << "    ldr w0, [x28, #" << info.offset << "]\n";
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
        generateExpr(*unaryExpr->operand);

        if (unaryExpr->op == "-") {
            out_ << "    neg w0, w0\n";
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
        int tempOffset = currentTempOffset();

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

            generateExpr(*callExpr->arguments[0]);
            out_ << "    mov w1, w0\n";
            out_ << "    adrp x0, .Lprint_fmt\n";
            out_ << "    add x0, x0, :lo12:.Lprint_fmt\n";
            out_ << "    bl printf\n";
            out_ << "    mov w0, #0\n";
            return;
        }

        if (callExpr->arguments.size() > 8) {
            error("more than 8 call arguments not supported yet");
        }

        std::vector<int> argOffsets;
        argOffsets.reserve(callExpr->arguments.size());

        int savedTempDepth = tempDepth_;

        for (const auto &arg : callExpr->arguments) {
            int argOffset = currentTempOffset();
            generateExpr(*arg);
            out_ << "    str w0, [x28, #" << argOffset << "]\n";
            argOffsets.push_back(argOffset);
            tempDepth_++;
        }

        for (std::size_t i = 0; i < argOffsets.size(); ++i) {
            out_ << "    ldr w" << i << ", [x28, #" << argOffsets[i] << "]\n";
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