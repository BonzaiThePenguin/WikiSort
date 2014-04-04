WikiSort
======

WikiSort is an implementation of "block merge sort", which is a stable merge sort based on the work described in ["Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner](http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF]. Block merge sort, or "block sort" for short, generally maintains **75-95%** of the speed of merge sort *while using O(1) memory*, and is even faster when the input is partially ordered. Block sort can also be modified to use any additional memory *optionally* provided to it, which can further improve its speed.

[C](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c), [C++](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp), and [Java](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.java) versions are currently available, and you have permission from me and the authors of the paper (Dr. Kim and Dr. Kutzner) to [do whatever you want with this code](https://github.com/BonzaiThePenguin/WikiSort/blob/master/LICENSE).

* * *

**If you want to learn how it works, check out the documentation:**<br/>
&nbsp;&nbsp;• [Chapter 1: Tools](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201.%20Tools.md)<br/>
&nbsp;&nbsp;• [Chapter 2: Merging](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202.%20Merging.md)<br/>
&nbsp;&nbsp;• [Chapter 3: In-Place](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203.%20In-Place.md)<br/>
&nbsp;&nbsp;• [Chapter 4: Faster!](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%204.%20Faster!.md)

Or you can check out the work-in-progress version of the [Wikipedia page](https://en.wikipedia.org/wiki/Wikipedia_talk:Articles_for_creation/Block_Sort) that is currently waiting for review. Technically block sort is still a merge sort despite *requiring* the use of some other sorting algorithm like insertion sort to function properly, but it was too detailed to justify putting in the existing in-place merge sort page.<br/>

* * *

**WikiSort vs. std::stable_sort():**

Using a 512-item fixed-size cache for O(1) memory:

    Random:             82% as fast
    RandomFew:          83% as fast
    MostlyDescending:   77% as fast
    MostlyAscending:   106% faster
    Ascending:         472% faster
    Descending:         99% as fast
    Equal:             474% faster
    Jittered:          266% faster
    MostlyEqual:        92% as fast

Using a dynamically allocated O(n) cache:

    Random:             97% as fast
    RandomFew:          95% as fast
    MostlyDescending:    9% faster
    MostlyAscending:   125% faster
    Ascending:         475% faster
    Descending:         20% faster
    Equal:             519% faster
    Jittered:          321% faster
    MostlyEqual:        99% as fast
