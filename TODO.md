To do
=================

Insertion sort is only faster when comparisons are cheap. Generally you'll want to use highly-optimized compare functions anyway, but ideally this algorithm should switch back to standard merges when it detects comparisons being too slow for insertion sort to help.

=================

At the moment, mostly-descending is noticeably slower than mostly-ascending. This algorithm would benefit from the following addition:

    #define wikisort(array, array_count, compare) { \
       __typeof__(array[0]) temp, *wikisort_array = array; const long wikisort_count = array_count; \
       uint64 i, ascending = 0, descending = 0; \
       for (i = 1; i < wikisort_count; i++) if (compare(wikisort_array[i], wikisort_array[i - 1]) >= 0) ascending++; else descending++; \
       if (descending > ascending) { reverse(wikisort_array); swap(ascending, descending); } \
       if (wikisort_count < 32) { \
          /* insertion sort the array */ \
          [...] \
       } else if (descending > 0) { \

However, a naïve array reverse operation would invalidate the stability of the algorithm:

&nbsp;&nbsp;&nbsp;[5 a] [5 b] [4 c]  <--->  [4 c] <b>[5 b] [5 a]</b>

A more-intelligent reversal that maintains the order of equivalent elements is needed before this can be added to the algorithm. Another possibility is simply copying-and-pasting a slightly-modified version of the algorithm that runs if descending > ascending, which is optimized for arrays whose elements are mostly in descending order.
