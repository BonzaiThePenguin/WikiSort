Chapter 4: Helping
=======================

You've <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Tools.md">read the documentation</a>, you've <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c">downloaded the public domain implementation</a>, and you totally understand how it works. Now what?

Well, have you thought about adding your own contributions? Sorting is a core operation of pretty much any program ever written, yet there are so few <i>good</i> and <i>documented</i> implementatons out there. Helping improve WikiSort would be a great way to benefit as many developers as possible!

=======================

Insertion sort is only faster when comparisons are cheap. Generally you'll want to use highly-optimized compare functions anyway, but ideally this algorithm should switch back to standard merges when it detects comparisons being too slow for insertion sort to help.

=======================

<a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203:%20In-Place.md">Redistributing the unique block values</a> back into the array after each merge step is a bit silly considering we're just going to pull them out again on the higher-up merge operation. This is probably the most obvious way to get a noticeable speed boost with the existing implementation, but it might not be trivial to implement.

=======================

Check the <a href="https://github.com/BonzaiThePenguin/WikiSort/issues">Issues</a> sidebar for other things that may need fixing.
