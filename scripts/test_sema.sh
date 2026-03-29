#!/usr/bin/env bash
set -u

TEST_DIR="examples/sema_fail"
COMPILER="./build/caracal"

mkdir -p "$TEST_DIR"

fail_count=0
pass_count=0

expect_check_failure() {
    local file="$1"
    local must_contain="$2"

    local base
    base="$(basename "$file")"

    echo "[TEST] $base"

    local output
    if output="$("$COMPILER" "$file" --check 2>&1)"; then
        echo "  FAIL: expected semantic failure but command succeeded"
        echo "$output"
        fail_count=$((fail_count + 1))
        return
    fi

    if printf '%s' "$output" | grep -Fq "$must_contain"; then
        echo "  PASS"
        pass_count=$((pass_count + 1))
    else
        echo "  FAIL: expected error containing:"
        echo "    $must_contain"
        echo "  actual output:"
        echo "$output"
        fail_count=$((fail_count + 1))
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
echo "Running semantic failure tests"
echo "=================================================="

expect_check_failure "$TEST_DIR/undeclared_variable.cr" "use of undeclared variable 'x'"
expect_check_failure "$TEST_DIR/undeclared_assignment.cr" "assignment to undeclared variable 'x'"
expect_check_failure "$TEST_DIR/duplicate_variable.cr" "duplicate variable declaration 'x'"
expect_check_failure "$TEST_DIR/duplicate_function.cr" "duplicate function declaration 'main'"
expect_check_failure "$TEST_DIR/wrong_arg_count.cr" "function 'add' expects 2 argument(s), got 1"
expect_check_failure "$TEST_DIR/undeclared_function.cr" "call to undeclared function 'foo'"

echo
echo "=================================================="
echo "Summary"
echo "=================================================="
echo "Passed: $pass_count"
echo "Failed: $fail_count"

if [ "$fail_count" -ne 0 ]; then
    exit 1
fi

echo "All sema failure tests passed."