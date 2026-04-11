fn main() -> i32 {
    let nums: [i32; 4] = [1, 2, 3, 4];
    print(nums[1]);
    nums[1] = 10;
    print(nums[1]);
    return nums[1];
}
