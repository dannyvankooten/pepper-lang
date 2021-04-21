// Output: Part 2: 157667328\nPart 1: 437931
// https://adventofcode.com/2020/day/1

// Read puzzle input from file
let input = file_get_contents("examples/aoc2020-day1.input");
input = str_split(input, "\n");

// Solve part 1 and 2 simultaneously 
for (let i=0; i < len(input); i = i + 1) {
    let a = int(input[i]);

    for (let j = i + 1; j < len(input); j = j + 1) {
        let b = int(input[j]);
        
        if (a + b == 2020) {
            puts("Part 1: ", a * b);
        }

        for (let k = j + 1; k < len(input); k = k + 1) {
            let c = int(input[k]);

            if (a + b + c == 2020) {
                puts("Part 2: ", a * b * c);
                break;
            }
        }
    }
}
