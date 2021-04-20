let input = file_get_contents("01.input");
input = str_split(input, "\n");

for (let i=0; i < len(input); i = i + 1) {
    for (let j = i + 1; j < len(input); j = j + 1) {
        let a = int(input[i]);
        let b = int(input[j]);

        if (a + b == 2020) {
            puts(a * b);
            break;
        }
    }
}