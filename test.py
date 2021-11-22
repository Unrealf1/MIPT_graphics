block_size = 8

arr = [i for i in range(block_size * block_size)]
sums = []
for block in range(block_size):
    start = block * block_size
    end = block * block_size + block_size
    print("\tblock:", arr[start:end])
    print("\tblock sum:", sum(arr[start:end]))
    print("\tblock scan:", [ sum( [arr[start + l] for l in range(L)]  ) for L in range(block_size) ] )
    sums.append(sum(arr[start:end]))


print("\n\n\nsums:", sums)

for i in range(1, len(sums)):
    sums[i] += sums[i - 1]
print("scanned sums:", sums)


for i in range(1, len(arr)):
    arr[i] += arr[i - 1]
print("\n\n\nres:", arr)