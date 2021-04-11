let square = fn(x) {
    return x * x;
}

let a = 10;
let b = 23;

let result = square(a) + b * b - 128;
puts(result);

if (result > 500) {
    puts("Result is larger than 500! Difference: ", result - 500);
}

