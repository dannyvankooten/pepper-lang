def fib(n)
  return n if n <= 1
  return fib(n - 1) + fib(n - 2)
end

puts "35th fibonacci number is: ", fib(35);
