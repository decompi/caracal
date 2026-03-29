#!/usr/bin/env bash
set -u

REMOTE="${CARACAL_REMOTE:-decompi@10.196.129.122}"
REMOTE_BIN_PATH="${CARACAL_REMOTE_BIN_PATH:-caracal_test_bin}"
BUILD_DIR="build/tests"
EXAMPLES_DIR="examples/codegen"
COMPILER="./build/caracal"

mkdir -p "$BUILD_DIR"

fail_count=0
pass_count=0

print_header() {
    echo
    echo "=================================================="
    echo "$1"
    echo "=================================================="
}

run_remote_binary() {
    local remote_path="$1"

    ssh "$REMOTE" "bash -lc '
        ./$remote_path > /tmp/caracal_stdout.txt 2>/tmp/caracal_stderr.txt
        status=\$?
        printf \"__CARACAL_EXIT_CODE__:%s\n\" \"\$status\"
        printf \"__CARACAL_STDOUT_BEGIN__\n\"
        cat /tmp/caracal_stdout.txt
        printf \"__CARACAL_STDOUT_END__\n\"
        printf \"__CARACAL_STDERR_BEGIN__\n\"
        cat /tmp/caracal_stderr.txt
        printf \"__CARACAL_STDERR_END__\n\"
        rm -f /tmp/caracal_stdout.txt /tmp/caracal_stderr.txt
    '"
}

check_exit_only() {
    local file="$1"
    local expected_exit="$2"

    local base
    base="$(basename "$file" .cr)"
    local asm_path="$BUILD_DIR/$base.s"
    local local_bin="$BUILD_DIR/$base"

    echo "[TEST] $base"

    if ! "$COMPILER" "$file" -o "$asm_path"; then
        echo "  FAIL: compiler failed"
        fail_count=$((fail_count + 1))
        return
    fi

    if ! aarch64-linux-gnu-gcc "$asm_path" -o "$local_bin"; then
        echo "  FAIL: assembler/link step failed"
        fail_count=$((fail_count + 1))
        return
    fi

    if ! scp "$local_bin" "$REMOTE:$REMOTE_BIN_PATH" >/dev/null; then
        echo "  FAIL: scp to remote failed"
        fail_count=$((fail_count + 1))
        return
    fi

    local remote_output
    if ! remote_output="$(run_remote_binary "$REMOTE_BIN_PATH")"; then
        echo "  FAIL: remote execution failed"
        fail_count=$((fail_count + 1))
        return
    fi

    local actual_exit
    actual_exit="$(printf '%s\n' "$remote_output" | sed -n 's/^__CARACAL_EXIT_CODE__://p')"

    if [ "$actual_exit" = "$expected_exit" ]; then
        echo "  PASS: exit code = $actual_exit"
        pass_count=$((pass_count + 1))
    else
        echo "  FAIL: expected exit $expected_exit, got $actual_exit"
        echo "$remote_output"
        fail_count=$((fail_count + 1))
    fi
}

check_stdout_and_exit() {
    local file="$1"
    local expected_stdout="$2"
    local expected_exit="$3"

    local base
    base="$(basename "$file" .cr)"
    local asm_path="$BUILD_DIR/$base.s"
    local local_bin="$BUILD_DIR/$base"

    echo "[TEST] $base"

    if ! "$COMPILER" "$file" -o "$asm_path"; then
        echo "  FAIL: compiler failed"
        fail_count=$((fail_count + 1))
        return
    fi

    if ! aarch64-linux-gnu-gcc "$asm_path" -o "$local_bin"; then
        echo "  FAIL: assembler/link step failed"
        fail_count=$((fail_count + 1))
        return
    fi

    if ! scp "$local_bin" "$REMOTE:$REMOTE_BIN_PATH" >/dev/null; then
        echo "  FAIL: scp to remote failed"
        fail_count=$((fail_count + 1))
        return
    fi

    local remote_output
    if ! remote_output="$(run_remote_binary "$REMOTE_BIN_PATH")"; then
        echo "  FAIL: remote execution failed"
        fail_count=$((fail_count + 1))
        return
    fi

    local actual_exit
    actual_exit="$(printf '%s\n' "$remote_output" | sed -n 's/^__CARACAL_EXIT_CODE__://p')"

    local actual_stdout
    actual_stdout="$(
        printf '%s\n' "$remote_output" |
        awk '
            /^__CARACAL_STDOUT_BEGIN__$/ { inside=1; next }
            /^__CARACAL_STDOUT_END__$/ { inside=0; exit }
            inside { print }
        '
    )"

    if [ "$actual_exit" = "$expected_exit" ] && [ "$actual_stdout" = "$expected_stdout" ]; then
        echo "  PASS: stdout and exit matched"
        pass_count=$((pass_count + 1))
    else
        echo "  FAIL:"
        echo "    expected exit:   $expected_exit"
        echo "    actual exit:     $actual_exit"
        echo "    expected stdout: [$expected_stdout]"
        echo "    actual stdout:   [$actual_stdout]"
        echo "$remote_output"
        fail_count=$((fail_count + 1))
    fi
}

print_header "Building compiler"
if ! ./scripts/build.sh; then
    echo "Build failed"
    exit 1
fi

print_header "Running codegen tests"

check_exit_only "$EXAMPLES_DIR/arithmetic_return.cr" "14"
check_exit_only "$EXAMPLES_DIR/locals_return.cr" "5"
check_exit_only "$EXAMPLES_DIR/if_else.cr" "11"
check_exit_only "$EXAMPLES_DIR/while.cr" "1"
check_exit_only "$EXAMPLES_DIR/return_const.cr" "10"
check_exit_only "$EXAMPLES_DIR/function_call_basic.cr" "5"
check_exit_only "$EXAMPLES_DIR/function_call_locals.cr" "11"

check_stdout_and_exit "$EXAMPLES_DIR/print_basic.cr" "7" "0"
check_stdout_and_exit "$EXAMPLES_DIR/print_call.cr" "7" "0"

print_header "Summary"
echo "Passed: $pass_count"
echo "Failed: $fail_count"

if [ "$fail_count" -ne 0 ]; then
    exit 1
fi

echo "All codegen tests passed."