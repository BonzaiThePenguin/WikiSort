Chapter 4: Helping
=======================

You've <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203:%20In-Place.md">read the documentation</a>, you've <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c">downloaded the public domain implementation</a>, and you totally understand how it works. Now what?

Well, have you thought about adding your own contributions? Sorting is a core operation of pretty much any program ever written, yet there are so few <i>good</i> and <i>documented</i> implementatons out there. Helping improve WikiSort would be a great way to benefit as many developers as possible!

=======================

While WikiSort is already quite fast, especially for an in-place merge sort (once thought to be of only <i>theoretical</i> interest), there are still a few things that it does not handle well. <i>Reversed</i> data in particular is probably the most common situation that can lead to supbar performance. This algorithm would benefit from the following addition:

    WikiSort(array, count, compare)
       order = 0
       for (index = 1; index < count; index++) order = order + compare(array[index], array[index - 1])
       if (order < 0) Reverse(array, MakeRange(0, count)) // the items are more in descending order, so reverse it

However, a naïve array <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Tools.md">reverse operation</a> would invalidate the stability of the algorithm:

&nbsp;&nbsp;&nbsp;[5 a] [5 b] [4 c]  <--->  [4 c] <b>[5 b] [5 a]</b>

A more-intelligent reversal that maintains the order of equivalent elements is needed before this can be added to the algorithm. Another possibility is creating a slightly modified duplicate of the sort function, one that works better for reversed data.

=======================

Insertion sort is only faster when comparisons are cheap. Generally you'll want to use highly-optimized compare functions anyway, but ideally this algorithm should switch back to standard merges when it detects comparisons being too slow for insertion sort to help.

=======================

<a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203:%20In-Place.md">Redistributing the unique block values</a> back into the array after each merge step is a bit silly considering we're just going to pull them out again on the higher-up merge operation. This is probably the most obvious way to get a noticeable speed boost with the existing implementation, but it might not be trivial to implement.
