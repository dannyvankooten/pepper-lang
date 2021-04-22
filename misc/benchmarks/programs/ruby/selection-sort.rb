file = File.open("numbers.txt")
str = file.read 
file.close 
arr = str.split(", ").map{ |string| string.to_i }

for i in 0..arr.length-1 do
    # find idx of min value
    min_idx = i
    for j in i..arr.length-1 do 
        if arr[j] < arr[min_idx] then
            min_idx = j
        end

    end

    # swap
    tmp = arr[i]
    arr[i] = arr[min_idx]
    arr[min_idx] = tmp
end

puts "%d - %d" % [ arr[0], arr[-1] ]