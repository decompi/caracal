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

CodeGenerator::CodeGenerator(std::ostream &out) : out_(out) {

}

void CodeGenerator::generate(const ast::Program &program) {
    for(const auto &fn: program.functions) {
        generateFunction(*fn);
    }
}

void CodeGenerator::generateFunction(const ast::FunctionDecl &fn) {
    if(fn.name != "main") {
        error("only codegen for main is supported right now");
    }

    // manual assembly strings 😭😭

    out_ << ".text" << std::endl;
    out_ << ".global main" << std::endl;
    out_ << ".align 2" << std::endl;
    out_ << "main:" << std::endl;

    // prologue
    out_ << "    stp x29, x30, [sp, -16]!" << std::endl;
    out_ << "    mov x29, sp" << std::endl;

    for (const auto &stmt : fn.body->statements) {
        generateStmt(*stmt);
    }

    // fallback return 0 if somehow no explicit return is hit
    out_ << "    mov w0, #0" << std::endl;
    out_ << "    ldp x29, x30, [sp], 16" << std::endl;
    out_ << "    ret" << std::endl;
}

void CodeGenerator::generateStmt(const ast::Stmt &stmt) {
    if (const auto *returnStmt = dynamic_cast<const ast::ReturnStmt*>(&stmt)) {
        generateExpr(*returnStmt->value);
        out_ << "    ldp x29, x30, [sp], 16" << std::endl;
        out_ << "    ret" << std::endl;
        return;
    }

    error("only return statements are supported in codegen right now");
}

void CodeGenerator::generateExpr(const ast::Expr &expr) {
    if (const auto *intExpr = dynamic_cast<const ast::IntegerExpr*>(&expr)) {
        out_ << "    mov w0, #" << intExpr->value << std::endl;
        return;
    }

    error("only integer literals are supported in codegen right now");
}

[[noreturn]] void CodeGenerator::error(const std::string &message) const {
    throw std::runtime_error("codegen error: " + message);
}