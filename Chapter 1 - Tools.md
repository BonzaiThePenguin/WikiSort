Chapter 1: Tools
================

Here are some useful functions that will be needed later:

* * *

**Swapping**

One function that is often needed is the ability to swap the values stored in
two variables. This is quite simple – just use a *third* variable to temporarily
hold one of the values, then reassign them.

    Swap(a, b)
        temp = a
        a = b
        b = temp

* * *

**Block swapping**

Block swapping is a convenience function for swapping multiple sequential items
in an array. So block swapping array[0] and array[10] with a block size of 5
would swap items 0-4 and 10-14.

    BlockSwap(array, index1, index2, count)
        for (index = 0; index < count; index++)
            Swap(array[index1 + index], array[index2 + index])

* * *

**Reversing**

To reverse the items in an array, simply swap the *n*th item with the
*(count - n)*th item, until we reach the middle of the array.

    Reverse(array, range)
        for (index = range.length/2 - 1; index >= 0; index--)
            Swap(array[range.start + index], array[range.end - index - 1])

* * *

**Rotating**

Rotating an array involves shifting all of the items over some number of
spaces, with items wrapping around to the other side as needed. So, for example,
[0 1 2 3 4] might become [2 3 4 0 1] after rotating it by 2.

Rotations can be implemented as three reverse operations, like so:

    Rotate(array, range, amount)
        Reverse(array, MakeRange(range.start, amount))
        Reverse(array, MakeRange(range.start + amount, range.end))
        Reverse(array, range)

Here's an example showing why and how this works:

    1. we have [0 1 2 3 4] and we want [2 3 4 0 1] by rotating by 2
    [0 1 2 3 4]

    2. reverse the first two items
    [1 0][2 3 4]

    3. reverse the last three items
    [1 0][4 3 2]

    4. reverse the entire array
    [2 3 4][0 1]

There's more to it than that (rotating in the other direction, rotating more
spaces than the size of the array, etc.), but that's the general idea. See the
[code](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp)
for the full implementation.

One important thing to note is that rotations can be applied to sections of an array, like so:

    1. we start with this:
    [0 1 2 3 4 5 6]

    2. let's rotate [2 3 4] to the left one
    [0 1 [2 3 4] 5 6]

    3. all done; note that [0 1] and [5 6] are still the same
    [0 1 [3 4 2] 5 6]

* * *

**Linear search**

Linear searching is nothing more than a simple loop through the items in an
array, looking for the index that matches the value we want. This is an O(n)
operation, meaning if there are n items in the array it has to check up to n
items before it finds it.

    LinearSearch(array, range, value)
        for (index = range.start; index < range.end; index++)
            if (array[index] == value)
                return index

* * *

**Binary search**

Binary searching works by checking the *middle* of the current range, and if
the value is greater it checks the right side, and if the value is less it
checks the left side. This repeats until there's only one item left. This is
an O(log n) operation, which is much more efficient than linear searching, but
it only works when the items in the array are in order. Fortunately since this
is a *sorting algorithm*, there are many situations where items in an array
will be in order!

This sorting algorithm uses two variants of the binary search, one for finding
the first place to insert a value in order, and the other for finding the *last*
place to insert the value. If the value does not yet exist in the array these two
functions return the same index, but otherwise the last index will be greater.

    BinaryFirst(array, range, value)
        start = range.start
        end = range.end
        while (start < end)
            mid = start + (end - start)/2
            if (array[mid] < value)
                start = mid + 1
            else
                end = mid
        if (start == range.end && array[start] < value) start = start + 1
        return start


Finding the last place to insert a value is identical, except we use <=

    BinaryLast(array, range, value)
        start = range.start
        end = range.end
        while (start < end)
            mid = start + (end - start)/2
            if (array[mid] <= value)
                start = mid + 1
            else
                end = mid
        if (start == range.end && array[start] <= value) start = start + 1
        return start

* * *

**Insertion sorting**

Hey, we're getting close to actual sorting now! Unfortunately, insertion sort
is an O(n^2) operation, which means it becomes *incredibly* inefficient for
large arrays. However, for small arrays it is actually quite fast, and is
therefore useful as part of a larger sorting algorithm.

    InsertionSort(array, range)
        for (i = range.start + 1; i < range.end; i++)
            temp = array[i]
            for (j = i; j > range.start && array[j - 1] > temp; j--)
                array[j] = array[j - 1]
            array[j] = temp

* * *

Speaking of larger sorting algorithms, [here's how merge sort works]
(https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md)!
