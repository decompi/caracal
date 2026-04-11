#!/usr/bin/env bash
set -u

COMPILER="./build/caracal"
EXAMPLE="examples/codegen/constant_fold.cr"
BUILD_DIR="build/opt_tests"

mkdir -p "$BUILD_DIR"

fail_count=0
pass_count=0

pass() {
    echo "  PASS: $1"
    pass_count=$((pass_count + 1))
}

fail() {
    echo "  FAIL: $1"
    fail_count=$((fail_count + 1))
}

expect_contains() {
    local haystack="$1"
    local needle="$2"
    local label="$3"

    if printf '%s\n' "$haystack" | grep -Fq "$needle"; then
        pass "$label"
    else
        fail "$label"
        echo "    expected to find: $needle"
    fi
}

expect_not_contains() {
    local haystack="$1"
    local needle="$2"
    local label="$3"

    if printf '%s\n' "$haystack" | grep -Fq "$needle"; then
        fail "$label"
        echo "    unexpectedly found: $needle"
    else
        pass "$label"
    fi
}

echo "=================================================="
echo "Building compiler"
echo "=================================================="
if ! ./scripts/build.sh; then
    echo "Build failed"
    exit 1
fi

echo
echo "=================================================="
echo "Checking optimized AST"
echo "=================================================="

ast_opt_output="$("$COMPILER" "$EXAMPLE" --ast-opt 2>&1)"
ast_status=$?

if [ "$ast_status" -ne 0 ]; then
    fail "optimized AST command failed"
    echo "$ast_opt_output"
else
    expect_contains "$ast_opt_output" "Integer(20)" "folded integer expression"
    expect_contains "$ast_opt_output" "Float(-4)" "folded float expression"
    expect_contains "$ast_opt_output" "Bool(true)" "folded bool expression"
    expect_not_contains "$ast_opt_output" "BinaryExpr(+)" "removed folded addition nodes"
    expect_not_contains "$ast_opt_output" "UnaryExpr(!)" "removed folded unary not node"
fi

echo
echo "=================================================="
echo "Comparing codegen output"
echo "=================================================="

NOOPT_ASM="$BUILD_DIR/constant_fold_noopt.s"
OPT_ASM="$BUILD_DIR/constant_fold_opt.s"

if ! "$COMPILER" "$EXAMPLE" -o "$NOOPT_ASM" >/dev/null 2>&1; then
    fail "unoptimized codegen failed"
elif ! "$COMPILER" "$EXAMPLE" --opt -o "$OPT_ASM" >/dev/null 2>&1; then
    fail "optimized codegen failed"
else
    noopt_ops="$(grep -n "fadd\|fneg\|add w0, w1, w0\|mul w0, w1, w0" "$NOOPT_ASM" || true)"
    opt_ops="$(grep -n "fadd\|fneg\|add w0, w1, w0\|mul w0, w1, w0" "$OPT_ASM" || true)"

    expect_contains "$noopt_ops" "add w0, w1, w0" "unoptimized integer add present"
    expect_contains "$noopt_ops" "mul w0, w1, w0" "unoptimized integer mul present"
    expect_contains "$noopt_ops" "fadd" "unoptimized float add present"
    expect_contains "$noopt_ops" "fneg" "unoptimized float neg present"

    if [ -z "$opt_ops" ]; then
        pass "optimized assembly removed folded runtime ops"
    else
        fail "optimized assembly still contains folded runtime ops"
        echo "$opt_ops"
    fi
fi

echo
echo "=================================================="
echo "Summary"
echo "=================================================="
echo "Passed: $pass_count"
echo "Failed: $fail_count"

if [ "$fail_count" -ne 0 ]; then
    exit 1
fi

echo "All optimization tests passed."