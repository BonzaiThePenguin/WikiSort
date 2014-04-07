WikiSort
======

WikiSort is an implementation of "block merge sort", which is a stable merge sort based on the work described in ["Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner](http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF]. Block merge sort, or "block sort" for short, is generally **as fast as a standard merge sort while using O(1) memory**, and is *even faster* when the input is partially ordered. Block sort can also be modified to use any additional memory *optionally* provided to it, which can further improve its speed.

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

    Test             Fast comparisons   Slow comparisons
    Random              99% as fast       94% as fast
    RandomFew            1% faster        19% faster
    MostlyDescending    95% as fast       16% faster
    MostlyAscending    150% faster       116% faster
    Ascending         1164% faster       487% faster
    Descending          22% faster       128% faster
    Equal             1158% faster       426% faster
    Jittered           470% faster       287% faster
    MostlyEqual         17% faster        60% faster

Using a dynamically allocated half-size cache:

    Test             Fast comparisons   Slow comparisons
    Random              11% faster         2% faster
    RandomFew           15% faster         6% faster
    MostlyDescending    38% faster        35% faster
    MostlyAscending     96% faster        80% faster
    Ascending          812% faster       450% faster
    Descending          75% faster       176% faster
    Equal              822% faster       462% faster
    Jittered           321% faster       244% faster
    MostlyEqual         18% faster         5% faster
