Chapter 2: Merging
=============

<b>Standard merge sort</b><br/>
Merge sorting works by breaking an array into two halves, over and over again, until the size of each half is below some threshold. For a threshold of 2 this means simply swapping the two items in that part of the array if they are out of order. For a larger threshold you could use an insertion sort or something. Once we have sorted two halves of the array, we have to <i>merge</i> them together to arrive at the final sorted array.<br/>

    MergeSort(array, range)
        if (range.length == 2)
            if (array[range.start] > array[range.end]) Swap(array[range.start], array[range.end])
        else
            mid = range.start + range.length/2
            MergeSort(array, MakeRange(range.start, mid))
            MergeSort(array, MakeRange(mid, range.end))
            Merge(array, MakeRange(range.start, mid), MakeRange(mid, range.end))

<br/><br/>
<b>Standard merge</b><br/>
The merge operation of the merge sort algorithm takes two arrays that <i>are already sorted</i>, and combines them into a single array containing A and B sorted together. The operation is acutally quite simple: just take the smaller of the two values at the start of A and B and add it to the final array. Once A and B are empty, the final array is completed!<br/>

    Merge(array, A, B)
        Copy A and B into a buffer
        A_count = 0, B_count = 0, insert = 0
        while (A_count < A.length && B_count < B.length)
            if (buffer[A_count] <= buffer[B.start + B_count])
                array[A.start + insert] = buffer[A_count]
                A_count = A_count + 1
            else
                array[A.start + insert] = buffer[B.start + B_count]
                B_count = B_count + 1
            insert = insert + 1
        Copy the remaining part of the buffer back into the array

Here's an example of how it works:
    
    1. we want to merge these two arrays, A and B:
    [0 2 4 7]  [1 3 7 8]
    []
    
    2. 0 is smaller
    [2 4 7]  [1 3 7 8]
    [0]
    
    3. 1 is smaller
    [2 4 7]  [3 7 8]
    [0 1]
    
    4. 2 is smaller
    [4 7]  [3 7 8]
    [0 1 2]
    
    5. 3 is smaller
    [4 7]  [7 8]
    [0 1 2 3]
    
    6. 4 is smaller
    [7]  [7 8]
    [0 1 2 3 4]
    
    7. 7 and 7 are equal, so give precedence to A
    []  [7 8]
    [0 1 2 3 4 7]
    
    8. A is empty, so just add the rest of B to the end
    [] []
    [0 1 2 3 4 7 7 8]

<br/><br/>
<b>Problems</b><br/>
There are some significant drawbacks to this design, especially if you're concerned about memory usage. The recursion actually uses O(n log n) stack space, and the merge operation requires a separate buffer that's the same size as the original array.<br/>

Can we do better? <b>Of course!</b><br/><br/>


<b>Merge sort without recursion</b><br/>
To remove the recursion, we can use what's called a <a href="http://www.algorithmist.com/index.php/Merge_sort#Bottom-up_merge_sort">bottom-up merge sort</a>. Here's what it looks like for an array of a size that happens to be a power of two:<br/>

    MergeSort(array, count)
       index = 0
       while (index < count)
          merge = index
          index += 2
          length = 1
          
          iteration = index
          while (iteration is even)
             start = merge
             mid = merge + length
             end = merge + length + length
             
             Merge(array, MakeRange(start, mid), MakeRange(mid, end))
             
             iteration = iteration / 2
             length = length * 2
             merge = merge - length

For an array of size 16, it does this (the operation is shown to the right):

                                       [ 15  2   13  7   3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 0-0 and 1-1                  [[15][2 ] 13  7   3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 2-2 and 3-3                  [ 2   15 [13][7 ] 3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 0-1 and 2-3                  [[2   15][7   13] 3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 4-4 and 5-5                  [ 2   7   13  15 [3 ][0 ] 11  4   12  6   10  14  1   9   8   5  ]
    merge 6-6 and 7-7                  [ 2   7   13  15  0   3  [11][4 ] 12  6   10  14  1   9   8   5  ]
    merge 4-5 and 6-7                  [ 2   7   13  15 [0   3 ][4   11] 12  6   10  14  1   9   8   5  ]
    merge 0-3 and 4-7                  [[2   7   13  15][0   3   4   11] 12  6   10  14  1   9   8   5  ]
    merge 8-8 and 9-9                  [ 0   2   3   4   7   11  13  15 [12][6 ] 10  14  1   9   8   5  ]
    merge 10-10 and 11-11              [ 0   2   3   4   7   11  13  15  6   12 [10][14] 1   9   8   5  ]
    merge 8-9 and 10-11                [ 0   2   3   4   7   11  13  15 [6   12][10  14] 1   9   8   5  ]
    merge 12-12 and 13-13              [ 0   2   3   4   7   11  13  15  6   10  12  14 [1 ][9 ] 8   5  ]
    merge 14-14 and 15-15              [ 0   2   3   4   7   11  13  15  6   10  12  14  1   9  [8 ][5 ]]
    merge 12-13 and 14-15              [ 0   2   3   4   7   11  13  15  6   10  12  14 [1   9 ][5   8 ]]
    merge 8-11 and 12-15               [ 0   2   3   4   7   11  13  15 [6   10  12  14][1   5   8   9 ]]
    merge 0-7 and 8-15                 [[0   2   3   4   7   11  13  15][1   5   6   8   9   10  12  14 ]
                                       [ 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15 ]
Which is of course exactly what we wanted.<br/><br/>

<b>To extend this logic to non-power-of-two sizes</b>, we simply floor the size down to the nearest power of two for these calculations, then scale back again to get the ranges to merge. Floating-point multiplications are blazing-fast these days so it hardly matters.

    MergeSort(array, count)
    >  power_of_two = FloorPowerOfTwo(count)
    >  scale = count/power_of_two // 1.0 <= scale < 2.0
       
       index = 0
       while (index < power_of_two)
          merge = index
          index += 2
          length = 1
          
          iteration = index
          while (iteration is even)
    >        start = merge * scale
    >        mid = (merge + length) * scale
    >        end = (merge + length + length) * scale
             
             Merge(array, MakeRange(start, mid), MakeRange(mid, end))
             
             iteration = iteration / 2
             length = length * 2
             merge = merge - length

The multiplication has been proven to be correct for more than 17,179,869,184 elements, which should be adequate. <b>This guarantees that the two ranges being merged will always have the same size to within one item, which makes it more efficient and allows for additional optimizations.</b><br/>

(note: the standard in-place merge sort uses min(end, count) rather than a floating-point multiplication, but this way was found to be faster in practice)<br/><br/>

==========================

Anyway, this removes the need for the O(n log n) stack space! But what about the merge operation?<br/><br/><br/>


<b>Merging using a half-size buffer</b><br/>
One obvious optimization is to only copy the values from A into the buffer, since by the time we run the risk of overwriting values in the range of B we will have already read and compared those values. Here's what this variant looks like:<br/>

    Merge(array, A, B)
        Copy the values from A into the buffer
        A_count = 0, B_count = 0, insert = 0
        while (A_count < A.length && B_count < B.length)
            if (buffer[A_count] <= array[B.start + B_count])
                array[A.start + insert] = buffer[A_count]
                A_count = A_count + 1
            else
                array[A.start + insert] = array[B.start + B_count]
                B_count = B_count + 1
            insert = insert + 1
        Copy the remaining part of the buffer back into the array

<br/><br/>
<b>Merging without overwriting the contents of the half-size buffer</b><br/>
Finally, if instead of assigning values we <i>swap</i> them to and from the buffer, we end up with a merge operation that still requires extra space, but doesn't overwrite any of the values that were stored in that extra space:<br/>

    Merge(array, A, B)
        Block swap the values in A with those in the buffer
        A_count = 0, B_count = 0, insert = 0
        while (A_count < A.length && B_count < B.length)
            if (buffer[A_count] <= array[B.start + B_count])
                Swap(array[A.start + insert], buffer[A_count])
                A_count = A_count + 1
            else
                Swap(array[A.start + insert], array[B.start + B_count])
                B_count = B_count + 1
            insert = insert + 1
        Block swap the remaining part of the buffer with the remaining part of the array

<br/><br/>
So now we can efficiently merge A and B without losing any buffer values, but we still need that additional buffer to exist. There isn't much we can do about that... but what if this extra buffer <i>was part of the original array</i>?<br/><br/>


That's the idea behind <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203:%20In-Place.md">in-place merging</a>!
