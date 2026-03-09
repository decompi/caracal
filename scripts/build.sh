#!/usr/bin/env bash
set -e

mkdir -p build

clang++ -std=c++20 -O2 -Wall -Wextra -pedantic \
  src/main.cpp \
  src/common/token.cpp \
  src/lexer/lexer.cpp \
  src/parser/parser.cpp \
  -o build/caracal