fn main() -> i32 {
    let a: [f64; 2] = [1.5, 2.5];
    let b: [f64; 2] = [10.0, 20.0];

    let outer: i32 = 0;
    let total: f64 = 0.0;

    while (outer < 4096) {
        let inner: i32 = 0;

        while (inner < 4096) {
            total = total + (a[0] * b[0]);
            total = total + (a[1] * b[1]);
            inner = inner + 1;
        }

        outer = outer + 1;
    }

    print(total);
    return 0;
}