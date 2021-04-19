// Variable declarations
let a = 5;
let b = 25;

// Redeclare existing variables
a = a * 5;
b == a; // True

// For loops
for (let i=0; i < 10; i = i + 1) {
    // i = 0 .. 9
}

// While loops
while (b > a) {
    b = b - 5;
}

// If statements
if (b == a) {
    puts("b equals a!");
}

// Strings
let c = "Hello world";
str_split(c, " "); // Array: [ "Hello", "world" ]

// Arrays
let d = [5, true, "Monkey"];
d[0]; // 5
array_push(d, 10);
array_pop(d); // 10

// Functions
let fibonacci = fn(x) {
    if (x < 2) {
        return x;
    }

    return fibonacci(x - 1) + fibonacci(x - 2);
}

// Built-in functions
puts("35th fibonacci number is: ", fibonacci(35));
puts("Type of c = ", type(c));
puts("Length of c = ", len(c));
puts("Integer value of true = ", int(true));