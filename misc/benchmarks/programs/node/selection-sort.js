let sort = function(arr) {
    for (let i=0; i < arr.length; i=i+1) {
        let min = i;

        // find minimum value
        for (let j=i+1; j < arr.length; j=j+1) {
            if (arr[j] < arr[min]) {
                min = j;
            }
        }

        // swap
        let tmp = arr[i];
        arr[i] = arr[min];
        arr[min] = tmp;
    }
    return arr;
};


const fs = require('fs')
data = fs.readFileSync('numbers.txt', 'utf8')
data = data.split(',').map(d => parseInt(d));

console.log(sort(data));