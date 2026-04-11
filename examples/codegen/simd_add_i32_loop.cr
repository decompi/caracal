fn main() -> i32 {
    let a: [i32; 4] = [1, 2, 3, 4];
    let b: [i32; 4] = [10, 20, 30, 40];
    let out: [i32; 4] = [0, 0, 0, 0];
    let i: i32 = 0;

    while(i < 4) {
        out[i] = a[i] + b[i];
        i = i + 1;
    }

    print(out[0]);
    return out[3];
}