fn main() -> i32 {
    let spot: f64 = 128.0;
    let strike: f64 = 120.0;
    let rate: f64 = 0.0625;
    let time: f64 = 1.0;
    let volatility: f64 = 0.25;

    let forward: f64 = spot + (spot * rate * time);
    let intrinsic: f64 = forward - strike;
    let volatilityAdjustment: f64 = spot * volatility * 0.5 * time;
    let callPrice: f64 = intrinsic + volatilityAdjustment;

    print(forward);
    print(intrinsic);
    print(callPrice);

    if (callPrice == 32.0) {
        return 0;
    }

    return 1;
}