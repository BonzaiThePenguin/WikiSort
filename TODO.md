To do
=================

The original version included a full test suite for generating arrays with various properties (random, mostly ascending, mostly descending, all equivalent, in order, reverse order, etc. etc.) and timing and verifying the correctness, but it was left out from this version as it uses a large number of dependencies. A self-contained benchmark and test tool would be very useful.

The merge step is pretty naïve at the moment and can fail miserably for arrays like this:

    [10 11 11 ...one million 11s... 11 | 9 9 ...one million 9s... 9 10]

It will happily copy one value at a time from the right side into swap, and shift the left side over when swap runs out of space... <b>1000 times in a row</b>. Better techniques for this subsystem are needed, or the O(1) memory usage will need to be sacrificed (while still retaining the benefits of being faster than similar algorithms).

At the moment, mostly-descending is noticeably slower than mostly-ascending. This algorithm would benefit from the following addition:

    #define bzSort(array, array_count, compare) { \
       __typeof__(array[0]) temp, *bzSort_array = array; const long bzSort_count = array_count; \
       uint64 i, ascending = 0, descending = 0; \
       for (i = 1; i < bzSort_count; i++) if (compare(bzSort_array[i], bzSort_array[i - 1]) >= 0) ascending++; else descending++; \
       if (descending > ascending) { reverse(bzSort_array); swap(ascending, descending); } \
       if (bzSort_count < 32) { \
          /* insertion sort the array */ \
          [...] \
       } else if (descending > 0) { \

However, a naïve array reverse operation would invalidate the stability of the algorithm:

&nbsp;&nbsp;&nbsp;[5 a] [5 b] [4 c]  <--->  [4 c] <b>[5 b] [5 a]</b>

A more-intelligent reversal that maintains the order of equivalent elements is needed before this can be added to the algorithm. Another possibility is simply copying-and-pasting a slightly-modified version of the algorithm that runs if descending > ascending, which is optimized for arrays whose elements are mostly in descending order.
