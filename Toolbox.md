Toolbox Functions
===================

Here are some useful functions that will be needed later:<br/><br/>

<b>Comparing</b><br/>
At the heart of any sorting algorithm is a function that compares two items in the array with each other. By convention, returning 0 means the two items are equal, +1 means the first item was greater than the second, and -1 means the first item was less than the second.<br/><br/>

    compare(a, b)
        if (a > b) return 1
        if (a < b) return -1
        return 0

<b>Swapping</b><br/>
One function that is often needed is the ability to swap the values stored in two variables. This is quite simple – just use a <i>third</i> variable to temporarily hold one of the values, then reassign them.<br/><br/>

    swap(a, b)
        temp = a
        a = b
        b = temp

<b>Block swapping</b><br/>
Block swapping is a convenience function for swapping multiple sequential items in an array. So block swapping array[0] and array[10] with a block size of 5 would swap items 0-4 and 10-14.

<b>Reversing</b><br/>
To reverse the items in an array, simply swap the <i>n</i>th item with the <i>(count - n)</i>th item, until we reach the middle of the array.

<b>Rotating</b><br/>
Rotating an array involves shifting all of the items over some number of spaces, with items wrapping around to the other side as needed. Rotations can be implemented as three reverse operations, like so:

    rotate(array, range, amount)
        reverse(array, MakeRange(range.start, range.end - amount))
        reverse(array, MakeRange(range.end - amount, range.end))
        reverse(array, range)

<b>Linear search</b><br/>

<b>Binary search</b><br/>
- find the first place to insert a value
- find the last place to insert a value

<b>Insertion sorting</b><br/>
Hey, we're getting close to actual sorting now! Unfortunately, insertion sort is an O(n^2) operation, which means it becomes <i>incredibly</i> inefficient for large arrays. However, for small arrays it is actually quite fast, and is therefore useful as part of a larger sorting algorithm.<br/><br/>

Speaking of larger sorting algorithms, <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/MergeSort.md">here's how merge sort works</a>!
