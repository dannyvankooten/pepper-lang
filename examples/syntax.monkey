// Variable declarations
let a = 5;
let b = 25;

// While statements
while (b > a) {
    let b = b - 5;
}

// If statements
if (b == a) {
    puts("b equals a!");
}

// Strings
let c = "Hello world";
puts("Type of c = ", type(c));
puts("Length of c = ", len(c));

// Functions
let fibonacci = fn(x) {
    if (x < 2) {
        return x;
    }

    return fibonacci(x - 1) + fibonacci(x - 2);
}

// Built-in functions
puts("35th fibonacci number is: ", fibonacci(35));