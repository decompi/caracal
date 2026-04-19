# Benchmarks

Right now, I'm benchmarking Caracal against the equivalent programs in C compiled for ARM64.

The point of these benchmarks is not to show that Carcal beats clang (which it doesn't). The goal is to make the generated code performance visible and document which compiler optimizations would help close the gap.

# Environment

All benchmarks were ran on ARM64 Raspberry Pi 3B Plus over SSH.

Compiler/toolchain:

- Caracal compiler was written in C++
- Caracl target: ARM64 assembly
- C baseline compiler: `aarch64-linux-gnu-gcc`
- Timing method: built in bash time method, remote execution using nanosecond timestamps from `date +%s%N`
- Benchmark runner: `scripts/bench_dot_product.sh`

# Dot Product f64 Benchmark

Source files:

- `examples/bench/dot_product_f64_bench.cr`
- `benchmarks/dot_product_f64.c`

Workload:

Basically just repeated double-precision dot-product style arithmetic over two element f64 arrays inside a 4096 * 4096 nested loop.

Expected output: `1090519040.000000`

## Results
| Program | Time |
| --- | ---: |
| Caracal | 1067 ms |
| C, `-O0` | 380 ms |
| C, `-O2` | 103 ms |

## Ineterpretation

Caracal is only generating correct native ARM64 code but it is still nowhere near what clang does once you turn on optimizations.

Most of the gap is just because the backend I have right now is still very simple. It moves a lot of values to the stakc instead of keeping it in registers, and there isn't a real register allocator. It also is not doing things like loop invariant code motion, common subexpression elimination... etc for this kind of scalar dot product loop.

Also, another reason is that I'm lowering pretty directly from the AST to assembly, instead of going through an IR that would make these optimizations easier.

So this result is pretty much what I would expect at this stage. The useful part is that the benchmark gives me a clear baseline and makes it obvious what I should be optimizing next.

## Current SIMD Support

Right now Caracal does support targeted ARM64 NEON lowering for specific array-add loops

These are the supported cases:
- `[i32; 4]` element-wise addition using `ld1`, `add v.4s`, and `st1`
- `[f64; 2]` element-wise addition using `ld1`, `fadd v.2d`, and `st1`

Example emitted NEON for `i32`:
```asm
ld1 {v0.4s}, [x9]
ld1 {v1.4s}, [x10]
add v2.4s, v0.4s, v1.4s
st1 {v2.4s}, [x11]
```

Example emmitted NEON for `f64`:
```asm
ld1 {v0.2d}, [x9]
ld1 {v1.2d}, [x10]
fadd v2.2d, v0.2d, v1.2d
st1 {v2.2d}, [x11]
```

The dot-product benchmark above is still measuring scalar codegen right now. Later on, I will add a version that compares Caracal's scalar and SIMD paths directly, since that will make it easier to see what the NEON lowering is actually.

## How to Run
Set the remote ARM64 target:
```bash
export CARACAL_REMOTE=user@host
```
Then run:
```bash
./scripts/bench_dot_product.sh
```
Example:
```bash
CARACAL_REMOTE=decompi@10.196.91.174 ./scripts/bench_dot_product.sh
```

## Next Optimization Targets

The main optimizations I want to do next are:
1. Add a simple register allocator for local scalar expressions
2. Keep loop counters in registers across loop iterations
3. Add loop-invariant code motion for repeated array base calcs.
4. Add dot-product-specific SIMD lowering using NEON multiply-add instructions.