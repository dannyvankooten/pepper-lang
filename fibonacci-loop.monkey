let fibonacci = fn(n) {
    let a = 0;
    let b = 1;
    let c = a + b;
    let i = 2;

    while (i < n) {
        let i = i + 1;
        let a = b;
        let b = c;
        let c = a + b;
    }

    return c;
}

puts("Fibonacci #5 is ", fibonacci(5));