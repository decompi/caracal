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

    if (out[0] == 11.5) {
        if (out[1] == 22.5) {
            return 0;
        }
    }

    return 1;
}