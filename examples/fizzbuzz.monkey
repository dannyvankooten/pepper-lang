for (let i=0; i <= 100; i=i+1) {
    let div3 = i % 3 == 0;
    let div5 = i % 5 == 0;
    
    if (div3 && div5) {
        puts("FizzBuzz");
        continue;
    }
    if (div3) {
        puts("Fizz");
        continue;
    }
    if (div5) {
        puts("Buzz");
        continue;
    }
    puts(i);
}