WikiSort
======

WikiSort is the first known implementation of "block merge sort", which is a stable merge sort based on the work described in ["Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner](http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF]. "Block sort" maintains 70-90% of the speed of a standard merge sort for highly random data *while using O(1) memory*, and is up to 10x faster when the input is already partially ordered. It also outpaces a standard merge sort once it needs to use virtual memory to allocate its O(n) buffer, continues working long after merge sort runs out of memory and crashes, and is 3-15x faster than other in-place merge sort algorithms.

WikiSort can also be modified to use any additional memory *optionally* provided to it, which can further improve its speed. When providing a full O(n) buffer, it'd turn into a full-speed merge sort. Instructions are provided in the source code.

**If you want to learn how it works, check out the documentation:**<br/>
&nbsp;&nbsp;• [Chapter 1: Tools](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201.%20Tools.md)<br/>
&nbsp;&nbsp;• [Chapter 2: Merging](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202.%20Merging.md)<br/>
&nbsp;&nbsp;• [Chapter 3: In-Place](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203.%20In-Place.md)<br/>
&nbsp;&nbsp;• [Chapter 4: Faster!](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%204.%20Faster!.md)

Or you can check out the work-in-progress version of the [Wikipedia page](https://en.wikipedia.org/wiki/Wikipedia_talk:Articles_for_creation/Block_Sort) that is currently waiting for review. Technically 'block merge sort' or 'block sort' is still a merge sort despite *requiring* the use of some other sorting algorithm like insertion sort to function properly, but it was too detailed to justify putting in the existing in-place merge sort page.<br/>

Anyway, [C](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c), [C++](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp), and [Java](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.java) versions are currently available, and you have permission from me and the authors of the paper (Dr. Kim and Dr. Kutzner) to do whatever you want with this code. This includes removing all references to WikiSort, BonzaiThePenguin, you name it – our only motivation is to take a field that used to be theoretical and impractical and turn it into something useful that everyone understands. Enjoy!
