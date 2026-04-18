fn main() -> i32 {
    let xs: [f64; 4] = [2.0, 4.0, 6.0, 8.0];

    let i: i32 = 0;
    let sum: f64 = 0.0;

    while (i < 4) {
        sum = sum + xs[i];
        i = i + 1;
    }

    let mean: f64 = sum / 4.0;

    let j: i32 = 0;
    let varianceSum: f64 = 0.0;

    while (j < 4) {
        let diff: f64 = xs[j] - mean;
        varianceSum = varianceSum + (diff * diff);
        j = j + 1;
    }

    let variance: f64 = varianceSum / 4.0;

    print(mean);
    print(variance);

    if (mean == 5.0) {
        if (variance == 5.0) {
            return 0;
        }
    }

    return 1;
}