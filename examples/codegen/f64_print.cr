fn add(x: f64, y: f64) -> f64 {
    return x + y;
}

fn main() -> i32 {
    let a: f64 = 3.5;
    let b: f64 = 2.25;
    let c: f64 = add(a, b);

    print(c);
    return 0;
}