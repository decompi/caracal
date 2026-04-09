fn add(x: f64, y: f64) -> f64 {
    let z: f64 = x + y;
    return z;
}

fn main() -> i32 {
    let a: f64 = 3.5;
    let b: f64 = 2.25;
    let c: f64 = add(a, b);

    if (c > 5.0) {
        return 0;
    }

    return 1;
}
