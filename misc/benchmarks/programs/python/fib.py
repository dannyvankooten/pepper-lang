
def fib(n):
    if n < 2:
        return n

    return fib(n-1) + fib(n-2)

print("35th fibonacci number is: ", fib(35))
