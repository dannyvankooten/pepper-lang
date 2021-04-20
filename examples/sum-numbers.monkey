let sum = 0;
let sum_even = 0;
for (let i=0; i < 10000000; i = i+1) {
	sum = sum + i;

	if (i % 2 == 0) {
		sum_even = sum_even + i;
	}
}
puts("Sum of all numbers = ", sum);
puts("Sum of all even numbers = ", sum_even);


