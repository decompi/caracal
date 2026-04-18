fn main() -> i32 {
    let a: [f64; 2] = [1.5, 2.5];
    let b: [f64; 2] = [10.0, 20.0];

    let i: i32 = 0;
    let sum: f64 = 0.0;

    while (i < 2) {
        sum = sum + (a[i] * b[i]);
        i = i + 1;
    }

    print(sum);

    if (sum == 65.0) {
        return 0;
    }

    return 1;
}