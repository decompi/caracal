fn less(x: i32, y: i32) -> bool {
    return x < y;
}

fn main() -> i32 {
    let ok: bool = less(2, 3);
    if (ok) {
        print(1);
    } else {
        print(0);
    }
    return 0;
}