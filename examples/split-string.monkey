// Output: 0: Comma\n1: separated\n2: values

let str = "Comma,separated,values";
let arr = str_split(str, ",");
for (let i=0; i < len(arr); i=i+1) {
    puts(i, ": ", arr[i]);
}