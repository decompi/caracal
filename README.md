# Caracal

Caracal is a statistically typed compiled language targeting ARM64. I'm building it from scratch in C++ mainly as a compiler/codegen project, which most of the work so far has focused on ARM64 assembly, floating point codegen and SIMD.

## Why this project exists

I wanted to design a compiler project where the main objective was performance sensitive software:

- type checking and semenatic correctness
- native code generation
- ARM64 calling conventions
- floating-point codegen
- stack frame layout
- bounds-checked arrays
- constant folding
- SIMD vectorization with ARM64 NEON
- numeric workloads and benchmarking

Basically, I wanted a real end-to-end compiler and to make the peformance tradeoffs visible

## Current Language Features

Cracal currently supports:

- `i32`
- `f64`
- `bool`
- fixed-size local arrays
- local variable declarations with `let`
- assignment
- indexed array reads and writes
- `if` / `else`
- `while`
- functions
- function calls
- `return`
- integer literals
- floating-point literals
- boolean literals
- arithmetic operators
- comparison and equality operators
- builtin `print`
- builtin `sqrt`
- builtin `exp`

Example:

```caracal
fn main() -> i32 {
    let a: [f64; 2] = [1.5, 2.5];
    let b: [f64; 2] = [10.0, 20.0];
    let out: [f64; 2] = [0.0, 0.0];
    let i: i32 = 0;

    while (i < 2) {
        out[i] = a[i] + b[i];
        i = i + 1;
    }

    print(out[0]);
    print(out[1]);
    return 0;
}
```

## Compiler Pipeline

The pipeline is pretty standard:
```text
source code
  -> lexer
  -> parser
  -> AST
  -> semantic analysis
  -> optional AST optimization
  -> ARM64 code generation
  -> assembler/linker
  -> native binary
```

Major source directories:

```text
src/lexer: tokenizes source code
src/parser: parses tokens into an AST
src/ast: AST node and type definitions
src/sema: semantic analysis and type checking
src/opt: AST optimization passes
src/codegen: ARM64 assembly generation
```

The CLI entry point is:

```text
src/main.cpp
```

## ARM64 Code Generation

Caracl emits native ARM64 assembly directly.

The backend currently handles:

- integer arithmetic with general-purpose registers
- floating-point arithmetic with scalar FP registers
- function calls using normal ARM64 calling conventions
- stack-allocated locals
- stack-allocated fixed-size arrays
- runtime bounds checks for array indexing
- `printf` interop for printing
- `fsqrt` lowering for `sqrt`
- libm call lowering for `exp`

Example scalar floating point codegen:
```asm
fmul d0, d1, d0
fadd d0, d1, d0
fsqrt d0, d0
```

## SIMD Vectorization

Right now Caracal has targeted ARM64 NEON lowering for simple element-wise array-add loops.

For `i32` arrays, Caracal recognizes this pattern:

```caracal
while (i < 4) {
    out[i] = a[i] + b[i];
    i = i + 1;
}
```

and emits 4-lane NEON instructions:

```asm
ld1 {v0.4s}, [x9]
ld1 {v1.4s}, [x10]
add v2.4s, v0.4s, v1.4s
st1 {v2.4s}, [x11]
```

For `f64` arrays, Caracal recognizes this pattern:

```caracal
while (i < 2) {
    out[i] = a[i] + b[i];
    i = i + 1;
}
```

and emits 2-lane NEON instructions:

```asm
ld1 {v0.2d}, [x9]
ld1 {v1.2d}, [x10]
fadd v2.2d, v0.2d, v1.2d
st1 {v2.2d}, [x11]
```

I had to change local array layout so numeric arrays are packed constiguously in memory so that the vector loads and stores were possible.

## Optimization

Right now I have small AST-level constant folding pass.


Use:

```bash
./build/caracal examples/codegen/constant_fold.cr --ast-opt
```

to inspect the optimized AST.

## Numeric Demos

There are already a few numeric examples in the repo that compile and run as native ARM64 binaries:

```text
examples/codegen/dot_product_f64.cr
examples/codegen/array_stats_f64.cr
examples/codegen/black_scholes_simplified.cr
examples/codegen/simd_add_i32_loop.cr
examples/codegen/simd_add_f64_loop.cr
```

These cover these things:
- Dot product
- Mean and variance
- Simplified option pricing style computation
- Scalar `f64` arithmetic
- NEON SIMD array addition
- `sqrt` and `exp` builtins


## Benchmarks

There is also a benchmark comparing generated ARM64 code against equivalent C built with different optimization levels.

Current dot product benchmark results:

```text
Caracal      1067 ms
C -O0         380 ms
C -O2         103 ms
```

Caracal emits correct native code, but it does not yet have a register allocator, loop optimizations, instruction scheduling, or a mature IR.

See:

```text
BENCHMARKS.md
```

Run the benchmark with:

```bash
CARACAL_REMOTE=user@host ./scripts/bench_dot_product.sh
```

## Build

Build the compiler:

```bash
./scripts/build.sh
```

This creates:

```text
build/caracal
```

## Usage

Compile a Caracal program to ARM64 assembly:

```bash
./build/caracal examples/codegen/f64_print.cr -o build/out.s
```

Print tokens:

```bash
./build/caracal examples/codegen/f64_print.cr --tokens
```

Print AST:

```bash
./build/caracal examples/codegen/f64_print.cr --ast
```

Run semantic analysis only:

```bash
./build/caracal examples/codegen/f64_print.cr --check
```

Run optimization before codegen:

```bash
./build/caracal examples/codegen/constant_fold.cr --opt -o build/out.s
```

Print optimized AST:

```bash
./build/caracal examples/codegen/constant_fold.cr --ast-opt
```

Enable SIMD lowering:

```bash
./build/caracal examples/codegen/simd_add_f64_loop.cr --simd -o build/simd.s
```

## Testing

Semantic tests:

```bash
./scripts/test_sema.sh
```

Codegen tests:

```bash
CARACAL_REMOTE=user@host ./scripts/test_codegen.sh
```

Optimization tests:

```bash
./scripts/test_opt.sh
```

SIMD tests:

```bash
CARACAL_REMOTE=user@host ./scripts/test_simd.sh
```

## Current Limitations

Caracal is still small and still pretty early. The main limitations right now are:

- No heap allocation
- No structs
- No strings
- No array parameters
- No array return types
- No general register allocator
- No full IR yet
- Limited SIMD pattern matching
- Bounds checks are always emitted
- Most optimization is still AST-level