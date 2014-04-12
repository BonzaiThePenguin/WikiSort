WikiSort
======

WikiSort is an implementation of "block merge sort", or "block sort" for short, which is a stable merge sort based on the work described in ["Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner](http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF].

WikiSort is generally **as fast as a standard merge sort while using O(1) memory**, and is *even faster* when the input is partially ordered or as the arrays get larger. It can also be modified to use any additional memory *optionally* provided to it, which can further improve its speed.

[C](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c), [C++](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp), and [Java](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.java) versions are currently available, and you have permission from me and the authors of the paper (Dr. Kim and Dr. Kutzner) to [do whatever you want with this code](https://github.com/BonzaiThePenguin/WikiSort/blob/master/LICENSE).

* * *

**If you want to learn how it works, check out the documentation:**<br/>
&nbsp;&nbsp;• [Chapter 1: Tools](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201.%20Tools.md)<br/>
&nbsp;&nbsp;• [Chapter 2: Merging](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202.%20Merging.md)<br/>
&nbsp;&nbsp;• [Chapter 3: In-Place](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203.%20In-Place.md)<br/>
&nbsp;&nbsp;• [Chapter 4: Faster!](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%204.%20Faster!.md)

Or you can check out the work-in-progress version of the [Wikipedia page](https://en.wikipedia.org/wiki/Wikipedia_talk:Articles_for_creation/Block_Sort) that is currently waiting for review. Technically block sort is still a merge sort despite *requiring* the use of some other sorting algorithm like insertion sort to function properly, but it was too detailed to justify putting in the existing in-place merge sort page.<br/>

* * *

**WikiSort vs. std::stable_sort()**  
<sup>(clang++ version 3.2, sorting 0 to 1.5 million items)</sup>

Using a 512-item fixed-size cache for O(1) memory:

    Test             Fast comparisons   Slow comparisons   150,000,000 items    0-32 items
    Random               6% faster        95% as fast         35% faster        45% faster
    RandomFew            5% faster        16% faster          20% faster        45% faster
    MostlyDescending    97% as fast       13% faster          99% as fast       53% faster
    MostlyAscending    149% faster       117% faster         286% faster        47% faster
    Ascending         1280% faster       518% faster        1101% faster       242% faster
    Descending          23% faster       121% faster          12% faster       164% faster
    Equal             1202% faster       418% faster        1031% faster       227% faster
    Jittered           526% faster       298% faster         733% faster        70% faster
    MostlyEqual         15% faster        57% faster          10% faster        42% faster
    Append             153% faster        90% faster         348% faster       112% faster

Using a dynamically allocated half-size cache:

    Test             Fast comparisons   Slow comparisons
    Random              11% faster         3% faster
    RandomFew           10% faster         5% faster
    MostlyDescending    19% faster        26% faster
    MostlyAscending     98% faster        79% faster
    Ascending          861% faster       463% faster
    Descending          39% faster       142% faster
    Equal              837% faster       460% faster
    Jittered           326% faster       243% faster
    MostlyEqual         15% faster         2% faster
    Append             159% faster        94% faster
