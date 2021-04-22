
function fib(n)
  if n < 2 then
    return n
  end
  return fib(n-1) + fib(n-2)
end

print("35th fibonacci number is: ", fib(35))
