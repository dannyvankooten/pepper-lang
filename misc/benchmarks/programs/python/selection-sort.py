def selection_sort(arr):
    for i in range(0, len(arr)):
        min_idx = i 
        for j in range(i+1, len(arr)):
            if arr[j] < arr[min_idx]:
                min_idx = j 

        tmp = arr[i]
        arr[i] = arr[min_idx]
        arr[min_idx] = tmp

file = open('numbers.txt',mode='r')
str = file.read()
file.close()

str = str.split(', ')
arr = list(map(int, str))
selection_sort(arr)
print(arr[0], " - ", arr[4999])