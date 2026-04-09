#!/usr/bin/env bash
set -e

mkdir -p build

clang++ -std=c++20 -O2 -Wall -Wextra -pedantic \
  src/main.cpp \
  src/common/token.cpp \
  src/lexer/lexer.cpp \
  src/parser/parser.cpp \
  src/ast/ast_printer.cpp \
  src/sema/sema.cpp \
  src/codegen/codegen.cpp \
  src/opt/constant_folder.cpp \
  -o build/caracal