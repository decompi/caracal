#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ast {
    struct Expr;
    struct Stmt;
    struct BlockStmt;
    struct FunctionDecl;
    struct Program;

    using ExprPtr = std::unique_ptr<Expr>;
    using StmtPtr = std::unique_ptr<StmtPtr>;
    using BlockStmtPtr = std::unique_ptr<BlockStmt>;
    using FunctionDeclPtr = std::unique_ptr<FunctionDecl>;

    // basic node types
    struct Expr {
        virtual ~Expr() = default;
    };
    struct Stmt {
        virtual ~Stmt() = default;
    };

    // expressions
    struct IntegerExpr : Expr {
        int value;

        explicit IntegerExpr(int value) : value(value) {
            // empty on purpose
        }
    };

    struct VariableExpr : Expr {
        std::string name;

        explicit VariableExpr(std::string name): name(std::move(name)) {

        }
    };

    struct UnaryExpr : Expr {
        std::string op;
        ExprPtr operand;

        UnaryExpr(std::string op, ExprPtr operand) : op(std::move(op)), operand(std::move(operand)) {

        }
    };

    struct BinaryExpr: Expr {
        std::string op;
        ExprPtr left, right;

        BinaryExpr(ExprPtr left, std::string op, ExprPtr right)
            : op(std::move(op)), left(std::move(left)), right(std::move(right)) {

        }
    };

    struct CallExpr : Expr {
        std::string callee;
        std::vector<ExprPtr> arguments;

        CallExpr(std::string callee, std::vector<ExprPtr> args)
            : callee(std::move(callee)),
                arguments(std::move(args)) {

        }
    };

    // statements

    struct LetStmt: Stmt {
        std::string name;
        std::string typeName;
        ExprPtr initializer;

        LetStmt(std::string name, std::string typeName, ExprPtr initializer)
            : name(std::move(name)),
            typeName(std::move(typeName)),
            initializer(std::move(initializer)) {}
    };

    struct AssignStmt : Stmt {
        std::string name;
        ExprPtr value;

        AssignStmt(std::string name, ExprPtr value) : name(std::move(name)), value(std::move(value)) {

        }
    };

    struct ExprStmt : Stmt {
        ExprPtr expr;
        
        explicit ExprStmt(ExprPtr expr) : expr(std::move(expr)) {

        }
    };

    struct ReturnStmt : Stmt {
        ExprPtr value;

        explicit ReturnStmt(ExprPtr value) : value(std::move(value)) {

        }
    };

    struct IfStmt : Stmt {
        ExprPtr condition;
        BlockStmtPtr thenBranch, elseBranch;

        IfStmt(ExprPtr condition, BlockStmtPtr thenBranch, BlockStmtPtr elseBranch) : condition(std::move(condition)),
        thenBranch(std::move(thenBranch)), elseBranch(std::move(elseBranch)) {

        }
    };

    struct WhileStmt : Stmt {
        ExprPtr condition;
        BlockStmtPtr body;

        WhileStmt(ExprPtr condition, BlockStmtPtr body) : condition(std::move(condition)), body(std::move(body)) {

        }
    };

    struct BlockStmt : Stmt {
        std::vector<StmtPtr> statements;
        BlockStmt() = default;

        explicit BlockStmt(std::vector<StmtPtr> statements) : statements(std::move(statements)) {

        }
    };

    struct ConstStmt : Stmt {

    };

    // top level

    struct Parameter {
        std::string name, typeName;

        Parameter(std::string name, std::string typeName) : name(std::move(name)), typeName(std::move(typeName)) {

        }
    };

    struct FunctionDecl {
        std::string name, returnType;
        std::vector<Parameter> params;
        BlockStmtPtr body;

        FunctionDecl(std::string name, std::vector<Parameter> params, std::string returnType, BlockStmtPtr body):
            name(std::move(name)),
            params(std::move(params)),
            returnType(std::move(returnType)),
            body(std::move(body)) {

        }
    };

    struct Program {
        std::vector<FunctionDeclPtr> functions;

        Program() = default;

        explicit Program(std::vector<FunctionDeclPtr> functions) : functions(std::move(functions)) {

        }
    };
};