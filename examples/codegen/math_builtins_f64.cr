fn main() -> i32 {
    let root: f64 = sqrt(9.0);
    let growth: f64 = exp(0.0);
    let total: f64 = root + growth;

    print(root);
    print(growth);
    print(total);

    if (total == 4.0) {
        return 0;
    }

    return 1;
}
