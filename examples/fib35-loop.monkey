// Output: 35th fibonacci number is: 9227465
let fibonacci = fn(n) {
    let a = 0;
    let b = 1;
    let c = a + b;
    let i = 2;

    while (i < n) {
        i = i + 1;
        a = b;
        b = c;
        c = a + b;
    }

    return c;
}

puts("35th fibonacci number is: ", fibonacci(35));