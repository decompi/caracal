/*

# Goal is to generate ARM64 assembly for this right now

fn main() -> i32 {
    return 5;
}

# build to arm64 with the command
aarch64-linux-gnu-gcc build/out.s -o build.out

# then copy to raspberry pi and run it
scp build/out decompi@caracal-pi:~/out
ssh decompi@caracl-pi '~/out; echo $?'

^ exit code should be 5

*/

#include "codegen.hpp"

#include <stdexcept>

CodeGenerator::CodeGenerator(std::ostream& out) : out_(out), nextLocalOffset_(16), tempDepth_(0), labelCounter_(0) {

}

void CodeGenerator::generate(const ast::Program& program) {
    for (const auto& fn : program.functions) {
        generateFunction(*fn);
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

int CodeGenerator::declareLocal(const std::string& name) {
    if (localScopes_.empty()) {
        error("internal codegen error: no active scope for local '" + name + "'");
    }

    auto& scope = localScopes_.back();
    if (scope.count(name)) {
        error("internal codegen error: duplicate local in current scope '" + name + "'");
    }

    int offset = nextLocalOffset_;
    nextLocalOffset_ += LOCAL_SLOT_SIZE;

    if (nextLocalOffset_ >= TEMP_BASE_OFFSET) {
        error("stack frame exhausted; increase FRAME_SIZE/TEMP_BASE_OFFSET");
    }

    scope[name] = offset;
    return offset;
}

int CodeGenerator::lookupLocal(const std::string& name) const {
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

std::string CodeGenerator::makeLabel(const std::string& prefix) {
    return ".L" + prefix + "_" + std::to_string(labelCounter_++);
}

void CodeGenerator::generateFunction(const ast::FunctionDecl& fn) {
    if (fn.name != "main") {
        error("only codegen for 'main' is supported right now");
    }

    localScopes_.clear();
    nextLocalOffset_ = 16;
    tempDepth_ = 0;
    currentReturnLabel_ = makeLabel("return");

    out_ << ".text\n";
    out_ << ".global main\n";
    out_ << ".align 2\n";
    out_ << "main:\n";

    out_ << "    stp x29, x30, [sp, -16]!\n";
    out_ << "    mov x29, sp\n";
    out_ << "    sub sp, sp, #" << FRAME_SIZE << "\n";
    out_ << "    mov x28, sp\n";

    pushScope();
    generateBlock(*fn.body);
    popScope();

    out_ << currentReturnLabel_ << ":\n";
    out_ << "    add sp, sp, #" << FRAME_SIZE << "\n";
    out_ << "    ldp x29, x30, [sp], 16\n";
    out_ << "    ret\n";
}

void CodeGenerator::generateBlock(const ast::BlockStmt& block) {
    pushScope();

    for (const auto& stmt : block.statements) {
        generateStmt(*stmt);
    }

    popScope();
}

void CodeGenerator::generateStmt(const ast::Stmt& stmt) {
    if (const auto* letStmt = dynamic_cast<const ast::LetStmt*>(&stmt)) {
        int offset = declareLocal(letStmt->name);
        generateExpr(*letStmt->initializer);
        out_ << "    str w0, [x28, #" << offset << "]\n";
        return;
    }

    if (const auto* assignStmt = dynamic_cast<const ast::AssignStmt*>(&stmt)) {
        int offset = lookupLocal(assignStmt->name);
        generateExpr(*assignStmt->value);
        out_ << "    str w0, [x28, #" << offset << "]\n";
        return;
    }

    if (const auto* exprStmt = dynamic_cast<const ast::ExprStmt*>(&stmt)) {
        generateExpr(*exprStmt->expr);
        return;
    }

    if (const auto* returnStmt = dynamic_cast<const ast::ReturnStmt*>(&stmt)) {
        generateExpr(*returnStmt->value);
        out_ << "    b " << currentReturnLabel_ << "\n";
        return;
    }

    if (const auto* ifStmt = dynamic_cast<const ast::IfStmt*>(&stmt)) {
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

    if (const auto* whileStmt = dynamic_cast<const ast::WhileStmt*>(&stmt)) {
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

    if (const auto* blockStmt = dynamic_cast<const ast::BlockStmt*>(&stmt)) {
        generateBlock(*blockStmt);
        return;
    }

    error("unsupported statement in codegen");
}

void CodeGenerator::generateExpr(const ast::Expr& expr) {
    if (const auto* intExpr = dynamic_cast<const ast::IntegerExpr*>(&expr)) {
        out_ << "    mov w0, #" << intExpr->value << "\n";
        return;
    }

    if (const auto* varExpr = dynamic_cast<const ast::VariableExpr*>(&expr)) {
        int offset = lookupLocal(varExpr->name);
        out_ << "    ldr w0, [x28, #" << offset << "]\n";
        return;
    }

    if (const auto* unaryExpr = dynamic_cast<const ast::UnaryExpr*>(&expr)) {
        generateExpr(*unaryExpr->operand);

        if (unaryExpr->op == "-") {
            out_ << "    neg w0, w0\n";
            return;
        }

        error("unsupported unary operator '" + unaryExpr->op + "'");
    }

    if (const auto* binaryExpr = dynamic_cast<const ast::BinaryExpr*>(&expr)) {
        int tempOffset = currentTempOffset();

        generateExpr(*binaryExpr->left);
        out_ << "    str w0, [x28, #" << tempOffset << "]\n";
        tempDepth_++;

        generateExpr(*binaryExpr->right);

        tempDepth_--;
        out_ << "    ldr w1, [x28, #" << tempOffset << "]\n";

        const std::string& op = binaryExpr->op;

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

    if (dynamic_cast<const ast::CallExpr*>(&expr)) {
        error("function call codegen is not supported yet");
    }

    error("unsupported expression in codegen");
}

[[noreturn]] void CodeGenerator::error(const std::string& message) const {
    throw std::runtime_error("codegen error: " + message);
}