fn main() -> i32 {
    let xs: [f64; 2] = [1.5, 2.5];
    xs[1] = xs[0] + xs[1];

    print(xs[1]);

    if (xs[1] == 4.0) {
        return 0;
    }

    return 1;
}