fn main() -> i32 {
    let spot: f64 = 100.0;
    let strike: f64 = 95.0;
    let rate: f64 = 0.05;
    let time: f64 = 1.0;
    let volatility: f64 = 0.20;

    let discount: f64 = exp(-(rate * time));
    let discountedStrike: f64 = strike * discount;
    let intrinsic: f64 = spot - discountedStrike;
    let volatilityAdjustment: f64 = spot * volatility * sqrt(time) * 0.10;
    let callPrice: f64 = intrinsic + volatilityAdjustment;

    print(discountedStrike);
    print(intrinsic);
    print(callPrice);

    if (callPrice > 11.5) {
        if (callPrice < 11.8) {
            return 0;
        }
    }

    return 1;
}
