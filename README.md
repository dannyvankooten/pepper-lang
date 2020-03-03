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
./bin/monkey fibonacci.monkey
```

Install the Monkey interpreter on your system:
```
make install
```

Build & run tests
```
make check
```