#include "ast_printer.hpp"

#include <iostream>
#include <typeinfo>

namespace ast {
    AstPrinter::AstPrinter(std::ostream &out) : out_(out) {

    }

    void AstPrinter::print(const Program &program) {
        for (const auto &fn : program.functions) {
            printFunctionDecl(*fn, 0);
        }
    }

    void AstPrinter::indent(int level) {
        for(int i = 0; i < level; i++) {
            out_ << " ";
        }
    }

    void AstPrinter::printFunctionDecl(const FunctionDecl &fn, int level) {
        indent(level);
        out_ << "FunctionDecl(" << fn.name << ")" << std::endl;
        
        indent(level+1);
        out_ << "Params" << std::endl;

        for(const auto &param : fn.params) {
            indent(level + 2);
            out_ << "Param(" << param.name << ": " << toString(param.type) << ")" << std::endl;
        }

        indent(level + 1);
        out_ << "ReturnType(" << toString(fn.returnType) << ")" << std::endl;

        printBlockStmt(*fn.body, level + 1);
    }

    void AstPrinter::printBlockStmt(const BlockStmt &block, int level) {
        indent(level);
        out_ << "Block" << std::endl;

        for(const auto &stmt: block.statements) {
            printStmt(*stmt, level + 1);
        }
    }

    void AstPrinter::printStmt(const Stmt& stmt, int level) {
        if (const auto* letStmt = dynamic_cast<const LetStmt*>(&stmt)) {
            indent(level);
            out_ << "LetStmt(" << letStmt->name << ": " << toString(letStmt->type) << ")\n";
            printExpr(*letStmt->initializer, level + 1);
            return;
        }

        if (const auto* assignStmt = dynamic_cast<const AssignStmt*>(&stmt)) {
            indent(level);
            out_ << "AssignStmt(" << assignStmt->name << ")\n";
            printExpr(*assignStmt->value, level + 1);
            return;
        }

        if (const auto* indexAssignStmt = dynamic_cast<const IndexAssignStmt*>(&stmt)) {
            indent(level);
            out_ << "IndexAssignStmt(" << indexAssignStmt->arrayName << ")\n";

            indent(level + 1);
            out_ << "Index\n";
            printExpr(*indexAssignStmt->index, level + 2);

            indent(level + 1);
            out_ << "Value\n";
            printExpr(*indexAssignStmt->value, level + 2);
            return;
        }


        if (const auto* exprStmt = dynamic_cast<const ExprStmt*>(&stmt)) {
            indent(level);
            out_ << "ExprStmt\n";
            printExpr(*exprStmt->expr, level + 1);
            return;
        }

        if (const auto* returnStmt = dynamic_cast<const ReturnStmt*>(&stmt)) {
            indent(level);
            out_ << "ReturnStmt\n";
            printExpr(*returnStmt->value, level + 1);
            return;
        }

        if (const auto* ifStmt = dynamic_cast<const IfStmt*>(&stmt)) {
            indent(level);
            out_ << "IfStmt\n";

            indent(level + 1);
            out_ << "Condition\n";
            printExpr(*ifStmt->condition, level + 2);

            indent(level + 1);
            out_ << "Then\n";
            printBlockStmt(*ifStmt->thenBranch, level + 2);

            if (ifStmt->elseBranch) {
                indent(level + 1);
                out_ << "Else\n";
                printBlockStmt(*ifStmt->elseBranch, level + 2);
            }
            return;
        }

        if (const auto* whileStmt = dynamic_cast<const WhileStmt*>(&stmt)) {
            indent(level);
            out_ << "WhileStmt\n";

            indent(level + 1);
            out_ << "Condition\n";
            printExpr(*whileStmt->condition, level + 2);

            indent(level + 1);
            out_ << "Body\n";
            printBlockStmt(*whileStmt->body, level + 2);
            return;
        }

        if (const auto* blockStmt = dynamic_cast<const BlockStmt*>(&stmt)) {
            printBlockStmt(*blockStmt, level);
            return;
        }

        indent(level);
        out_ << "UnknownStmt\n";
    }

    void AstPrinter::printExpr(const Expr& expr, int level) {
        if (const auto* intExpr = dynamic_cast<const IntegerExpr*>(&expr)) {
            indent(level);
            out_ << "Integer(" << intExpr->value << ")\n";
            return;
        }

        if (const auto *floatExpr = dynamic_cast<const FloatExpr*>(&expr)) {
            indent(level);
            out_ << "Float(" << floatExpr->value << ")\n";
            return;
        }

        if (const auto* varExpr = dynamic_cast<const VariableExpr*>(&expr)) {
            indent(level);
            out_ << "Variable(" << varExpr->name << ")\n";
            return;
        }

        if (const auto* unaryExpr = dynamic_cast<const UnaryExpr*>(&expr)) {
            indent(level);
            out_ << "UnaryExpr(" << unaryExpr->op << ")\n";
            printExpr(*unaryExpr->operand, level + 1);
            return;
        }

        if (const auto* binaryExpr = dynamic_cast<const BinaryExpr*>(&expr)) {
            indent(level);
            out_ << "BinaryExpr(" << binaryExpr->op << ")\n";
            printExpr(*binaryExpr->left, level + 1);
            printExpr(*binaryExpr->right, level + 1);
            return;
        }

        if (const auto* arrayLiteralExpr = dynamic_cast<const ArrayLiteralExpr*>(&expr)) {
            indent(level);
            out_ << "ArrayLiteral\n";
            for (const auto &element : arrayLiteralExpr->elements) {
                printExpr(*element, level + 1);
            }
            return;
        }

        if (const auto* indexExpr = dynamic_cast<const IndexExpr*>(&expr)) {
            indent(level);
            out_ << "IndexExpr\n";

            indent(level + 1);
            out_ << "Base\n";
            printExpr(*indexExpr->base, level + 2);

            indent(level + 1);
            out_ << "Index\n";
            printExpr(*indexExpr->index, level + 2);
            return;
        }


        if (const auto *callExpr = dynamic_cast<const CallExpr*>(&expr)) {
            indent(level);
            out_ << "CallExpr(" << callExpr->callee << ")\n";
            for (const auto& arg : callExpr->arguments) {
                printExpr(*arg, level + 1);
            }
            return;
        }

        indent(level);
        out_ << "UnknownExpr\n";
    }
}