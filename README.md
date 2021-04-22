# Pepper Programming Language

<img src="https://raw.githubusercontent.com/dannyvankooten/pepper-lang/master/misc/logo.png" width="120" height="120" align="right" />

Pepper is an experimental interpreted programming language with a focus on simplicity and verbosity. It is dynamically typed and comes with built-in garbage collection. 

It's also [quite fast](#Benchmarks) for an interpreted language that does not (yet) rely on JIT compilation.

Pepper is implemented in standard C11 with no external dependencies, so it can run on a wide variety of systems.

### Syntax example 

```js
// Variable declarations
let a = 5;
let b = 25;

// Reassign (previously declared) variables
a = a * 5;

// For loops
for (let i = 0; i < 10; i = i + 1) {
    // i = 0 .. 9
}

// While loops
while (b > a) {
    b = b - 5;
    break;
}

// If statements
if (b == a || a == b) {
    puts("b equals a!");
}

// Strings
let c = "Hello world";
c = str_split(c, " "); 
type(c); // "ARRAY"
len(c); // 2 

// Arrays
let d = [5, true, "Pepper"];
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

```

More examples can be found in the [examples](https://github.com/dannyvankooten/pepper-lang/tree/master/examples) directory.

### Usage

Build Pepper interpreter (and REPL)
```
make 
```

Launch the REPL
```
bin/pepper
```

Interpret a Pepper script: 
```
bin/pepper examples/arithmetic.pr
```

Build & run tests
```
make check
```

Install Pepper in your system's binaries
```
sudo make install
```

### Benchmarks

A benchmark to calculate the [35th fibonacci number](https://github.com/dannyvankooten/pepper-lang/blob/master/examples/fib35-recursive.pr) using a recursive function is run on every commit through [this](https://github.com/dannyvankooten/pepper-lang/actions/workflows/c.yml) Github action workflow.

![Fibonacci 35 benchmark](https://raw.githubusercontent.com/dannyvankooten/pepper-lang/master/misc/benchmarks/chart.jpg)

For fun, I ran the same algorithm expressed in some other interpreted languages on the same hardware (my laptop). This is how Pepper compares:

| Language   	| fib-35   	| string-concat-count 	|
|------------	|----------	|---------------------	|
| Node 15    	| 0.21     	| 0.058                	|
| Pypy 7.3   	| 0.24     	| 0.154               	|
| PHP 8.0    	| 0.48     	| 0.036               	|
| **Pepper** 	| **0.72** 	| **0.024**           	|
| Lua 5.4    	| 0.83     	| 0.030                 |
| Ruby 3.0   	| 0.80     	| 0.102               	|
| Python 3.9 	| 1.91     	| 0.060               	|
| PHP 5.6    	| 2.72     	|                     	|


### License

MIT licensed. 