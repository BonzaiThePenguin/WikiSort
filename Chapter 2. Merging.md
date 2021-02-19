Chapter 2: Merging
==================

**Standard merge sort**

Merge sorting works by breaking an array into two halves, over and over again,
until the size of each half is below some threshold. For a threshold of 2 this
means simply swapping the two items in that part of the array if they are out
of order. For a larger threshold you could use an insertion sort or something.
Once we have sorted two halves of the array, we have to *merge* them together
to arrive at the final sorted array.

    MergeSort(array, range)
        if (range.length == 2)
            if (array[range.start] > array[range.end]) Swap(array[range.start], array[range.end])
        else if (range.length > 2)
            mid = range.start + range.length/2
            MergeSort(array, MakeRange(range.start, mid))
            MergeSort(array, MakeRange(mid, range.end))

            Merge(array, MakeRange(range.start, mid), MakeRange(mid, range.end))

* * *

**Standard merge**

The merge operation of the merge sort algorithm takes two arrays that *are
already sorted* (either from swapping or insertion sorting, as mentioned above),
and combines them into a single array containing A and B sorted together.
The operation is acutally quite simple: just take the smaller of the two values
at the start of A and B and add it to the final array. Once A and B are empty,
the final array is completed!

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
    [0 2 4 7]  [1 3 7 8]  []

    2. 0 is smaller
    [0 2 4 7]  [1 3 7 8]  []
     ^          ^

    3. 1 is smaller
    [2 4 7]  [1 3 7 8]  [0]
     ^        ^

    4. 2 is smaller
    [2 4 7]  [3 7 8]  [0 1]
     ^        ^

    5. 3 is smaller
    [4 7]  [3 7 8]  [0 1 2]
     ^      ^

    6. 4 is smaller
    [4 7]  [7 8]  [0 1 2 3]
     ^      ^

    7. 7 and 7 are equal, so give precedence to A
    [7]  [7 8]  [0 1 2 3 4]
     ^    ^

    8. A is empty, so just add the rest of B to the end
    []  [7 8]  [0 1 2 3 4 7]
         ^

    9. A + B have been merged!
    []  []  [0 1 2 3 4 7 7 8]

**Problems**

There are some significant drawbacks to this design, especially if you're
concerned about memory usage. The recursion actually uses O(log n) stack space,
and the merge operation requires a separate buffer that's the same size as
the original array.

Can we do better? **Of course!**

* * *

**Merge sort without recursion**

To remove the recursion, we can use what's called a [bottom-up merge sort](http://www.algorithmist.com/index.php/Merge_sort#Bottom-up_merge_sort). Here's
what it looks like for an array of a size that happens to be a power of two:

    MergeSort(array, count)
        for (length = 1; length < count; length = length * 2)
            for (merge = 0; merge < count; merge = merge + length * 2)
                start = merge
                mid = merge + length
                end = merge + length * 2

                Merge(array, MakeRange(start, mid), MakeRange(mid, end))

For an array of size 16, it does this (the operation is shown to the right):

                                       [ 15  2   13  7   3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 0-0 and 1-1                  [[15][2 ] 13  7   3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 2-2 and 3-3                  [ 2   15 [13][7 ] 3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 4-4 and 5-5                  [ 2   15  7   13 [3 ][0 ] 11  4   12  6   10  14  1   9   8   5  ]
    merge 6-6 and 7-7                  [ 2   15  7   13  0   3  [11][4 ] 12  6   10  14  1   9   8   5  ]
    merge 8-8 and 9-9                  [ 2   15  7   13  0   3   4   11 [12][6 ] 10  14  1   9   8   5  ]
    merge 10-10 and 11-11              [ 2   15  7   13  0   3   4   11  6   12 [10][14] 1   9   8   5  ]
    merge 12-12 and 13-13              [ 2   15  7   13  0   3   4   11  6   12  10  14 [1 ][9 ] 8   5  ]
    merge 14-14 and 15-15              [ 2   15  7   13  0   3   4   11  6   12  10  14  1   9  [8 ][5 ]]
    merge 0-1 and 2-3                  [[2   15][7   13] 0   3   4   11  6   12  10  14  1   9   5   8  ]
    merge 4-5 and 6-7                  [ 2   7   13  15 [0   3 ][4   11] 6   12  10  14  1   9   5   8  ]
    merge 8-9 and 10-11                [ 2   7   13  15  0   3   4   11 [6   12][10  14] 1   9   5   8  ]
    merge 12-13 and 14-15              [ 2   7   13  15  0   3   4   11  6   10  12  14 [1   9 ][5   8 ]]
    merge 0-3 and 4-7                  [[2   7   13  15][0   3   4   11] 6   10  12  14  1   5   8   9  ]
    merge 8-11 and 12-15               [ 0   2   3   4   7   11  13  15 [6   10  12  14][1   5   8   9 ]]
    merge 0-7 and 8-15                 [[0   2   3   4   7   11  13  15][1   5   6   8   9   10  12  14]]
                                       [ 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15 ]

Which is of course exactly what we wanted. Note that it starts off by merging
chunks of size 1, then size 2, then size 4, and finally size 8.

* * *

**To extend this logic to non-power-of-two sizes**, we floor the size down to
the nearest power of two for these calculations, then scale back again to get
the ranges to merge.

    MergeSort(array, count)
    >   power_of_two = FloorPowerOfTwo(count)
    >   scale = count/power_of_two // 1.0 <= scale < 2.0

        for (length = 16; length < power_of_two; length = length * 2)
            for (merge = 0; merge < power_of_two; merge = merge + length * 2)
                start = merge * scale
                mid = (merge + length) * scale
                end = (merge + length * 2) * scale

                Merge(array, MakeRange(start, mid), MakeRange(mid, end))

The floating-point multiplication was verified as correct for over 17 billion
items (as in there was no unexpected roundoff error), but if that seems a bit
flaky to you it's also possible to handle the scaling with integer operations:

    MergeSort(array, count)
        power_of_two = FloorPowerOfTwo(count)
        fractional_base = power_of_two/16
        fractional_step = size % fractional_base (modulus, which gets the remainder from size/fractional_base)
        decimal_step = floor(size/fractional_base)

        for (length = 16; length < power_of_two; length = length * 2)
            decimal = fractional = 0
            while (decimal < size)
                start = decimal

                decimal = decimal + decimal_step
                fractional = fractional + fractional_step
                if (fractional >= fractional_base)
                    fractional = fractional - fractional_base
                    decimal = decimal + 1

                mid = decimal

                decimal = decimal + decimal_step
                fractional = fractional + fractional_step
                if (fractional >= fractional_base)
                    fractional = fractional - fractional_base
                    decimal = decimal + 1

                end = decimal

                Merge(array, MakeRange(start, mid), MakeRange(mid, end))
            
            decimal_step = decimal_step * 2
            fractional_step = fractional_step * 2
            if (fractional_step >= fractional_base)
                fractional_step = fractional_step - fractional_base
                decimal_step = decimal_step + 1

This is considerably more involved than the standard bottom-up design, but it
guarantees that the two ranges being merged will always have the same size to
within one item, **which ends up being about 10% faster**.

This removes the need for the O(log n) stack space! Although, to be fair, that
wasn't really an issue – sorting 1.5 million items only recurses ~20 times
anyway. The real problem is that O(n) space for the separate buffer! What can
we do about *that*?

* * *

**Merging using a half-size buffer**

One simple optimization is to only copy the values from A into the buffer, since
by the time we run the risk of overwriting values in the range of B we will have
already read and compared those values. Here's what this variant looks like:

    Merge(array, A, B)
        Copy the values from A into the buffer, but leave B where it is
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

* * *

**Merging without overwriting the contents of the half-size buffer**

Finally, if instead of assigning values we *swap* them to and from the buffer,
we end up with a merge operation that still requires extra space, but doesn't
overwrite any of the values that were stored in that extra space:

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

The items in the buffer will likely be in a different order afterwards,
but at least they're still there.

* * *

So now we can efficiently merge A and B without losing any buffer values, but
we still need that additional buffer to exist. There isn't much we can do about
that... but what if this extra buffer *was part of the original array*?

That's the idea behind [in-place merging](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203.%20In-Place.md)!
