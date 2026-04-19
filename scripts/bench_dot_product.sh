#!/usr/bin/env bash
set -u

REMOTE="${CARACAL_REMOTE:-}"
REMOTE_DIR="${CARACAL_REMOTE_DIR:-caracal_benchmarks}"
COMPILER="./build/caracal"
BUILD_DIR="build/benchmarks"

CARACAL_SRC="examples/bench/dot_product_f64_bench.cr"
C_SRC="benchmarks/dot_product_f64.c"

if [ -z "$REMOTE" ]; then
    echo "error: CARACAL_REMOTE is not set"
    echo "usage: CARACAL_REMOTE=user@host ./scripts/bench_dot_product.sh"
    exit 1
fi

mkdir -p "$BUILD_DIR"

print_header() {
    echo
    echo "=================================================="
    echo "$1"
    echo "=================================================="
}

run_remote_timed() {
    local remote_bin="$1"

    ssh "$REMOTE" "bash -lc '
        start=\$(date +%s%N)
        ~/$REMOTE_DIR/$remote_bin >/tmp/caracal_bench_stdout.txt
        status=\$?
        end=\$(date +%s%N)
        elapsed_ns=\$((end - start))
        elapsed_ms=\$((elapsed_ns / 1000000))
        printf \"__EXIT__:%s\n\" \"\$status\"
        printf \"__MS__:%s\n\" \"\$elapsed_ms\"
        printf \"__STDOUT_BEGIN__\n\"
        cat /tmp/caracal_bench_stdout.txt
        printf \"__STDOUT_END__\n\"
        rm -f /tmp/caracal_bench_stdout.txt
    '"
}

extract_field() {
    local field="$1"
    sed -n "s/^__${field}__://p"
}

extract_stdout() {
    awk '
        /^__STDOUT_BEGIN__$/ { inside=1; next }
        /^__STDOUT_END__$/ { inside=0; exit }
        inside { print }
    '
}

benchmark_one() {
    local label="$1"
    local remote_bin="$2"
    local expected_stdout="$3"

    local output
    output="$(run_remote_timed "$remote_bin")"

    local status
    status="$(printf '%s\n' "$output" | extract_field "EXIT")"

    local elapsed_ms
    elapsed_ms="$(printf '%s\n' "$output" | extract_field "MS")"

    local stdout
    stdout="$(printf '%s\n' "$output" | extract_stdout)"

    if [ "$status" != "0" ]; then
        echo "$label failed with exit $status"
        echo "$output"
        return 1
    fi

    if [ "$stdout" != "$expected_stdout" ]; then
        echo "$label produced unexpected output"
        echo "expected: [$expected_stdout]"
        echo "actual:   [$stdout]"
        return 1
    fi

    printf "%-16s %8s ms\n" "$label" "$elapsed_ms"
}

print_header "Building compiler"
./scripts/build.sh || exit 1

print_header "Compiling benchmarks"

"$COMPILER" "$CARACAL_SRC" -o "$BUILD_DIR/dot_product_caracal.s" || exit 1
aarch64-linux-gnu-gcc "$BUILD_DIR/dot_product_caracal.s" -o "$BUILD_DIR/dot_product_caracal" || exit 1

aarch64-linux-gnu-gcc -O0 "$C_SRC" -o "$BUILD_DIR/dot_product_c_O0" || exit 1
aarch64-linux-gnu-gcc -O2 "$C_SRC" -o "$BUILD_DIR/dot_product_c_O2" || exit 1

print_header "Uploading benchmarks"

ssh "$REMOTE" "mkdir -p ~/$REMOTE_DIR" || exit 1
scp \
    "$BUILD_DIR/dot_product_caracal" \
    "$BUILD_DIR/dot_product_c_O0" \
    "$BUILD_DIR/dot_product_c_O2" \
    "$REMOTE:~/$REMOTE_DIR/" >/dev/null || exit 1

print_header "Running benchmarks"

EXPECTED="1090519040.000000"

benchmark_one "Caracal" "dot_product_caracal" "$EXPECTED" || exit 1
benchmark_one "clang -O0" "dot_product_c_O0" "$EXPECTED" || exit 1
benchmark_one "clang -O2" "dot_product_c_O2" "$EXPECTED" || exit 1