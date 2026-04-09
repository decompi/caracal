#pragma once

#include "../ast/ast.hpp"

struct ConstantFolder {
    void run(ast::Program &program);
};