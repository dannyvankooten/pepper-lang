# C implementation of the Monkey programming language.

<img src="https://monkeylang.org/images/logo.png" width="120" height="120"/>

Bytecode compiler and virtual machine for the [Monkey programming language](https://monkeylang.org), written in C.

This is the result of going through the wonderful books [Writing An Interpreter In Go](https://interpreterbook.com/) and [Writing A Compiler In Go](https://compilerbook.com/), but using a different language instead to squeeze more learning opportunities out of it and force myself to really understand what's going on.

Since I like uselessly optimizing for performance, it runs [quite fast](#Benchmarks) for something that doesn't do JIT compilation.

### Syntax example 

```
let fibonacci = fn(x) {
    if (x < 2) {
        return x;
    }

    return fibonacci(x - 1) + fibonacci(x - 2);
}

puts("35th fibonacci number is: ", fibonacci(35));
```

More examples can be found in the [examples](https://github.com/dannyvankooten/monkey-c-monkey-do/tree/master/examples) directory.

### Usage

Build Monkey interpreter (and REPL)
```
make 
```

Launch the REPL
```
./bin/monkey
```

Interpret a Monkey script: 
```
./bin/monkey examples/fib35.monkey
```

Build & run tests
```
make check
```

### Benchmarks

A benchmark to calculate the [35th fibonacci number](https://github.com/dannyvankooten/monkey-c-monkey-do/blob/master/examples/fib35.monkey) using a recursive function is run on every commit through [this](https://github.com/dannyvankooten/monkey-c-monkey-do/actions/workflows/c.yml) Github action workflow.

![Fibonacci 35 benchmark](https://raw.githubusercontent.com/dannyvankooten/monkey-c-monkey-do/master/misc/benchmarks.jpg)

For fun, I ran the same algorithm expressed in some other interpreted languages on the same hardware (my laptop). This is how Monkey-C compares:

| Language 	            | Time (s)	|
|--------------------	|------	|
| Node 15            	| 0.21 	|
| PHP 8.0            	| 0.79 	|
| Monkey-C-Monkey-Do 	| 0.99	|
| Lua 5.4            	| 1.19 	|
| Ruby 2.7           	| 1.29 	|
| Python 3.9         	| 3.08 	|


### License

GPLv3 