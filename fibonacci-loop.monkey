let a = 0;
let b = 1;
let c = a + b;
let n = 2;

while (n < 35) {
    let n = n + 1;
    let a = b;
    let b = c;
    let c = a + b;
}

puts(c);