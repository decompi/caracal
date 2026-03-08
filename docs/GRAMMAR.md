# Caracal Language Grammar


## 1. Functions
Structure:
  fn [NAME] () -> [TYPE] {
      [STATEMENTS...]
  }

## 2. Statements
Only 2 diff types so far

### A. Create a (Let) Variable
  let [NAME]: [TYPE] = [VALUE];

  Example:
  let x: i32 = 5;

### B. Return a Value
  return [VALUE];
  
  Example:
  return x;
  return 5;

## 3. Allowed Types
* `i32` (Integer, 32-bit)