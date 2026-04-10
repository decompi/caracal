#include "constant_folder.hpp"

#include <memory>
#include <string>

namespace {
    bool foldIntegerBinary(const std::string &op, int left, int right, ast::ExprPtr &expr) {
        if(op == "+") {
            expr = std::make_unique<ast::IntegerExpr>(left + right);
            return true;
        }

        if(op == "-") {
            expr = std::make_unique<ast::IntegerExpr>(left - right);
            return true;
        }

        if(op == "*") {
            expr = std::make_unique<ast::IntegerExpr>(left * right);
            return true;
        }

        if(op == "/") {
            if(right == 0) {
                return false;
            }
            expr = std::make_unique<ast::IntegerExpr>(left / right);
            return true;
        }

        if (op == "%") {
            if (right == 0) {
                return false;
            }
            expr = std::make_unique<ast::IntegerExpr>(left % right);
            return true;
        }

        if (op == "<") {
            expr = std::make_unique<ast::BoolExpr>(left < right);
            return true;
        }

        if (op == "<=") {
            expr = std::make_unique<ast::BoolExpr>(left <= right);
            return true;
        }

        if (op == ">") {
            expr = std::make_unique<ast::BoolExpr>(left > right);
            return true;
        }

        if (op == ">=") {
            expr = std::make_unique<ast::BoolExpr>(left >= right);
            return true;
        }

        if (op == "==") {
            expr = std::make_unique<ast::BoolExpr>(left == right);
            return true;
        }

        if (op == "!=") {
            expr = std::make_unique<ast::BoolExpr>(left != right);
            return true;
        }

        return false;
    }

    bool foldFloatBinary(const std::string &op, double left, double right, ast::ExprPtr &expr) {
        if (op == "+") {
            expr = std::make_unique<ast::FloatExpr>(left + right);
            return true;
        }

        if (op == "-") {
            expr = std::make_unique<ast::FloatExpr>(left - right);
            return true;
        }

        if (op == "*") {
            expr = std::make_unique<ast::FloatExpr>(left * right);
            return true;
        }

        if (op == "/") {
            expr = std::make_unique<ast::FloatExpr>(left / right);
            return true;
        }

        if (op == "<") {
            expr = std::make_unique<ast::BoolExpr>(left < right);
            return true;
        }

        if (op == "<=") {
            expr = std::make_unique<ast::BoolExpr>(left <= right);
            return true;
        }

        if (op == ">") {
            expr = std::make_unique<ast::BoolExpr>(left > right);
            return true;
        }

        if (op == ">=") {
            expr = std::make_unique<ast::BoolExpr>(left >= right);
            return true;
        }

        if (op == "==") {
            expr = std::make_unique<ast::BoolExpr>(left == right);
            return true;
        }

        if (op == "!=") {
            expr = std::make_unique<ast::BoolExpr>(left != right);
            return true;
        }

        return false;
    }

    void foldExpr(ast::ExprPtr &expr);

    void foldStmt(ast::StmtPtr &stmt);
    void foldBlock(ast::BlockStmt &block) {
        for (auto &stmt : block.statements) {
            foldStmt(stmt);
        }
    }

    void foldStmt(ast::StmtPtr &stmt) {
        if (const auto *letStmt = dynamic_cast<const ast::LetStmt*>(stmt.get())) {
            (void)letStmt;
        }

        if (auto *letStmt = dynamic_cast<ast::LetStmt*>(stmt.get())) {
            foldExpr(letStmt->initializer);
            return;
        }

        if (auto *assignStmt = dynamic_cast<ast::AssignStmt*>(stmt.get())) {
            foldExpr(assignStmt->value);
            return;
        }

        if (auto *indexAssignStmt = dynamic_cast<ast::IndexAssignStmt*>(stmt.get())) {
            foldExpr(indexAssignStmt->index);
            foldExpr(indexAssignStmt->value);
            return;
        }

        if (auto *exprStmt = dynamic_cast<ast::ExprStmt*>(stmt.get())) {
            foldExpr(exprStmt->expr);
            return;
        }

        if (auto *returnStmt = dynamic_cast<ast::ReturnStmt*>(stmt.get())) {
            foldExpr(returnStmt->value);
            return;
        }

        if (auto *ifStmt = dynamic_cast<ast::IfStmt*>(stmt.get())) {
            foldExpr(ifStmt->condition);
            foldBlock(*ifStmt->thenBranch);

            if (ifStmt->elseBranch) {
                foldBlock(*ifStmt->elseBranch);
            }
            return;
        }

        if (auto *whileStmt = dynamic_cast<ast::WhileStmt*>(stmt.get())) {
            foldExpr(whileStmt->condition);
            foldBlock(*whileStmt->body);
            return;
        }

        if (auto *blockStmt = dynamic_cast<ast::BlockStmt*>(stmt.get())) {
            foldBlock(*blockStmt);
            return;
        }
    }

    void foldExpr(ast::ExprPtr &expr) {
        if (auto *unaryExpr = dynamic_cast<ast::UnaryExpr*>(expr.get())) {
            foldExpr(unaryExpr->operand);

            if (unaryExpr->op == "-") {
                if (auto *intExpr = dynamic_cast<ast::IntegerExpr*>(unaryExpr->operand.get())) {
                    expr = std::make_unique<ast::IntegerExpr>(-intExpr->value);
                    return;
                }

                if (auto *floatExpr = dynamic_cast<ast::FloatExpr*>(unaryExpr->operand.get())) {
                    expr = std::make_unique<ast::FloatExpr>(-floatExpr->value);
                    return;
                }
            }

            if (unaryExpr->op == "!") {
                if (auto *boolExpr = dynamic_cast<ast::BoolExpr*>(unaryExpr->operand.get())) {
                    expr = std::make_unique<ast::BoolExpr>(!boolExpr->value);
                    return;
                }
            }

            return;
        }

        if (auto *binaryExpr = dynamic_cast<ast::BinaryExpr*>(expr.get())) {
            foldExpr(binaryExpr->left);
            foldExpr(binaryExpr->right);

            if (auto *leftInt = dynamic_cast<ast::IntegerExpr*>(binaryExpr->left.get())) {
                if (auto *rightInt = dynamic_cast<ast::IntegerExpr*>(binaryExpr->right.get())) {
                    foldIntegerBinary(binaryExpr->op, leftInt->value, rightInt->value, expr);
                    return;
                }
            }

            if (auto *leftFloat = dynamic_cast<ast::FloatExpr*>(binaryExpr->left.get())) {
                if (auto *rightFloat = dynamic_cast<ast::FloatExpr*>(binaryExpr->right.get())) {
                    foldFloatBinary(binaryExpr->op, leftFloat->value, rightFloat->value, expr);
                    return;
                }
            }

            return;
        }

        if (auto *callExpr = dynamic_cast<ast::CallExpr*>(expr.get())) {
            for (auto &arg : callExpr->arguments) {
                foldExpr(arg);
            }
            return;
        }

        if (auto *arrayLiteralExpr = dynamic_cast<ast::ArrayLiteralExpr*>(expr.get())) {
            for (auto &element : arrayLiteralExpr->elements) {
                foldExpr(element);
            }
            return;
        }

        if (auto *indexExpr = dynamic_cast<ast::IndexExpr*>(expr.get())) {
            foldExpr(indexExpr->base);
            foldExpr(indexExpr->index);
            return;
        }
    }
}

void ConstantFolder::run(ast::Program &program) {
    for (auto &fn : program.functions) {
        foldBlock(*fn->body);
    }
}