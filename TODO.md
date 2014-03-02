To do
=================

Insertion sort is only faster when comparisons are cheap. Generally you'll want to use highly-optimized compare functions anyway, but ideally this algorithm should switch back to standard merges when it detects comparisons being too slow for insertion sort to help.

=================

At the moment, mostly-descending is noticeably slower than mostly-ascending. This algorithm would benefit from the following addition:

    void WikiSort(WikiTest array[], const long array_count, WikiComparison compare) {
       long index, order = 0;
       for (index = 1; index < array_count; index++) order += compare(WikiItem(array, index), WikiItem(array, index - 1));
       if (order < 0) WikiReverse(array, WikiMakeRange(0, array_count)); // the items were in descending order, so reverse it

However, a naïve array reverse operation would invalidate the stability of the algorithm:

&nbsp;&nbsp;&nbsp;[5 a] [5 b] [4 c]  <--->  [4 c] <b>[5 b] [5 a]</b>

A more-intelligent reversal that maintains the order of equivalent elements is needed before this can be added to the algorithm. Another possibility is simply copying-and-pasting a slightly-modified version of the algorithm that runs if descending > ascending, which is optimized for arrays whose elements are mostly in descending order.
