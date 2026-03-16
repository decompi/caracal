fn main() -> i32 {
    let x: i32 = 5;
    if(x > 3) {
        let y: i32 = 1;
        x = x + y;
    }

    return x;
}