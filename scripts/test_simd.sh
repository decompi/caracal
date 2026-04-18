#!/usr/bin/env bash
set -u

REMOTE="${CARACAL_REMOTE:-}"
REMOTE_BIN_PATH="${CARACAL_REMOTE_BIN_PATH:-caracal_simd_test_bin}"
COMPILER="./build/caracal"
BUILD_DIR="build/simd_tests"

I32_EXAMPLE="examples/codegen/simd_add_i32_loop.cr"
F64_EXAMPLE="examples/codegen/simd_add_f64_loop.cr"

if [ -z "$REMOTE" ]; then
    echo "error: CARACAL_REMOTE is not set"
    echo "usage: CARACAL_REMOTE=user@host ./scripts/test_simd.sh"
    exit 1
fi

mkdir -p "$BUILD_DIR"

pass_count=0
fail_count=0

print_header() {
    echo
    echo "=================================================="
    echo "$1"
    echo "=================================================="
}

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

run_remote_binary() {
    local remote_path="$1"

    ssh "$REMOTE" "bash -lc '
        ./$remote_path > /tmp/caracal_simd_stdout.txt 2>/tmp/caracal_simd_stderr.txt
        status=\$?
        printf \"__CARACAL_EXIT_CODE__:%s\n\" \"\$status\"
        printf \"__CARACAL_STDOUT_BEGIN__\n\"
        cat /tmp/caracal_simd_stdout.txt
        printf \"__CARACAL_STDOUT_END__\n\"
        printf \"__CARACAL_STDERR_BEGIN__\n\"
        cat /tmp/caracal_simd_stderr.txt
        printf \"__CARACAL_STDERR_END__\n\"
        rm -f /tmp/caracal_simd_stdout.txt /tmp/caracal_simd_stderr.txt
    '"
}

extract_stdout() {
    awk '
        /^__CARACAL_STDOUT_BEGIN__$/ { inside=1; next }
        /^__CARACAL_STDOUT_END__$/ { inside=0; exit }
        inside { print }
    '
}

extract_exit_code() {
    sed -n 's/^__CARACAL_EXIT_CODE__://p'
}

compile_and_run() {
    local example="$1"
    local asm_path="$2"
    local bin_path="$3"
    local expected_stdout="$4"
    local expected_exit="$5"
    local label="$6"

    if ! "$COMPILER" "$example" --simd -o "$asm_path" >/dev/null 2>&1; then
        fail "$label codegen"
        return
    fi

    if ! aarch64-linux-gnu-gcc "$asm_path" -o "$bin_path"; then
        fail "$label link"
        return
    fi

    if ! scp "$bin_path" "$REMOTE:$REMOTE_BIN_PATH" >/dev/null; then
        fail "$label scp"
        return
    fi

    local remote_output
    if ! remote_output="$(run_remote_binary "$REMOTE_BIN_PATH")"; then
        fail "$label remote run"
        echo "$remote_output"
        return
    fi

    local actual_exit
    actual_exit="$(printf '%s\n' "$remote_output" | extract_exit_code)"

    local actual_stdout
    actual_stdout="$(printf '%s\n' "$remote_output" | extract_stdout)"

    if [ "$actual_exit" = "$expected_exit" ] && [ "$actual_stdout" = "$expected_stdout" ]; then
        pass "$label runtime output"
    else
        fail "$label runtime output"
        echo "    expected exit:   $expected_exit"
        echo "    actual exit:     $actual_exit"
        echo "    expected stdout: [$expected_stdout]"
        echo "    actual stdout:   [$actual_stdout]"
        echo "$remote_output"
    fi
}

print_header "Building compiler"
if ! ./scripts/build.sh; then
    echo "Build failed"
    exit 1
fi

print_header "Checking i32 SIMD assembly"

I32_SCALAR_ASM="$BUILD_DIR/simd_i32_scalar.s"
I32_SIMD_ASM="$BUILD_DIR/simd_i32_simd.s"
I32_BIN="$BUILD_DIR/simd_i32"

if "$COMPILER" "$I32_EXAMPLE" -o "$I32_SCALAR_ASM" >/dev/null 2>&1 &&
   "$COMPILER" "$I32_EXAMPLE" --simd -o "$I32_SIMD_ASM" >/dev/null 2>&1; then
    i32_scalar_neon="$(grep -n "ld1\\|st1\\|add v2.4s" "$I32_SCALAR_ASM" || true)"
    i32_simd_neon="$(grep -n "ld1\\|st1\\|add v2.4s" "$I32_SIMD_ASM" || true)"

    expect_not_contains "$i32_scalar_neon" "ld1" "i32 scalar assembly has no NEON loads"
    expect_contains "$i32_simd_neon" "ld1 {v0.4s}, [x9]" "i32 SIMD loads left vector"
    expect_contains "$i32_simd_neon" "ld1 {v1.4s}, [x10]" "i32 SIMD loads right vector"
    expect_contains "$i32_simd_neon" "add v2.4s, v0.4s, v1.4s" "i32 SIMD vector add"
    expect_contains "$i32_simd_neon" "st1 {v2.4s}, [x11]" "i32 SIMD stores vector"
else
    fail "i32 assembly generation"
fi

print_header "Checking f64 SIMD assembly"

F64_SCALAR_ASM="$BUILD_DIR/simd_f64_scalar.s"
F64_SIMD_ASM="$BUILD_DIR/simd_f64_simd.s"
F64_BIN="$BUILD_DIR/simd_f64"

if "$COMPILER" "$F64_EXAMPLE" -o "$F64_SCALAR_ASM" >/dev/null 2>&1 &&
   "$COMPILER" "$F64_EXAMPLE" --simd -o "$F64_SIMD_ASM" >/dev/null 2>&1; then
    f64_scalar_neon="$(grep -n "ld1\\|st1\\|fadd v2.2d" "$F64_SCALAR_ASM" || true)"
    f64_simd_neon="$(grep -n "ld1\\|st1\\|fadd v2.2d" "$F64_SIMD_ASM" || true)"

    expect_not_contains "$f64_scalar_neon" "ld1" "f64 scalar assembly has no NEON loads"
    expect_contains "$f64_simd_neon" "ld1 {v0.2d}, [x9]" "f64 SIMD loads left vector"
    expect_contains "$f64_simd_neon" "ld1 {v1.2d}, [x10]" "f64 SIMD loads right vector"
    expect_contains "$f64_simd_neon" "fadd v2.2d, v0.2d, v1.2d" "f64 SIMD vector add"
    expect_contains "$f64_simd_neon" "st1 {v2.2d}, [x11]" "f64 SIMD stores vector"
else
    fail "f64 assembly generation"
fi

print_header "Running SIMD binaries on remote ARM64"

compile_and_run "$I32_EXAMPLE" "$I32_SIMD_ASM" "$I32_BIN" "11" "44" "i32 SIMD"
compile_and_run "$F64_EXAMPLE" "$F64_SIMD_ASM" "$F64_BIN" "$(printf '11.500000\n22.500000')" "0" "f64 SIMD"

print_header "Summary"
echo "Passed: $pass_count"
echo "Failed: $fail_count"

if [ "$fail_count" -ne 0 ]; then
    exit 1
fi

echo "All SIMD tests passed."