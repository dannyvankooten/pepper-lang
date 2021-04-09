# C implementation of the Monkey programming language.

<img src="https://monkeylang.org/images/logo.png" width="120" height="120"/>

Interpreter for the [Monkey programming language](https://monkeylang.org), written in C.

This is my first C project, so expect very few best practices.

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

| Monkey-C-Monkey-Do 	| 1.08 	|
|--------------------	|------	|
| Node 15            	| 0.21 	|
| PHP 8.0            	| 0.79 	|
| Monkey-C-Monkey-Do 	| 1.08 	|
| Lua 5.4            	| 1.19 	|
| Ruby 2.7           	| 1.29 	|
| Python 3.9         	| 3.08 	|


### License

GPLv3 