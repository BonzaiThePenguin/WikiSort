WikiSort
======

WikiSort is an implementation of "block merge sort", which is a stable merge sort based on the work described in ["Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner](http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF]. Block merge sort, or "block sort" for short, generally maintains **80-95%** of the speed of merge sort *while using O(1) memory*, and is even faster when the input is partially ordered. Block sort can also be modified to use any additional memory *optionally* provided to it, which can further improve its speed.

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
<sup>(clang++ version 3.2)</sup>

Using a 512-item fixed-size cache for O(1) memory:

    Test             Fast comparisons   Slow comparisons
    Random              83% as fast       88% as fast
    RandomFew           84% as fast        7% faster
    MostlyDescending    82% as fast        5% faster
    MostlyAscending    111% faster       112% faster
    Ascending          892% faster       794% faster
    Descending          18% faster       176% faster
    Equal              932% faster       711% faster
    Jittered           390% faster       351% faster
    MostlyEqual         95% as fast       48% faster

Using a dynamically allocated half-size cache:

    Test             Fast comparisons   Slow comparisons
    Random              10% faster         2% faster
    RandomFew           12% faster         5% faster
    MostlyDescending    33% faster        31% faster
    MostlyAscending    101% faster        89% faster
    Ascending         1064% faster       857% faster
    Descending          84% faster       285% faster
    Equal             1162% faster       887% faster
    Jittered           366% faster       336% faster
    MostlyEqual         16% faster         8% faster

* * *

**WikiSort vs. std::__inplace_stable_sort()**  
<sup>(clang++ version 3.2)</sup>

Using a 20-item fixed-size cache to somewhat match __inplace_stable_sort's memory use:

    Test             Fast comparisons   Slow comparisons
    Random             317% faster       111% faster
    RandomFew          182% faster        57% faster
    MostlyDescending    92% faster        18% faster
    MostlyAscending     69% as fast       82% as fast
    Ascending          153% faster       199% faster
    Descending         201% faster       215% faster
    Equal               38% faster        69% faster
    Jittered            48% faster        71% faster
    MostlyEqual         42% faster        20% faster
