WikiSort
======

WikiSort is a stable bottom-up in-place merge sort based on the work described in ["Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner](http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF]. Kim's and Kutzner's algorithm is a stable merge algorithm with great performance characteristics and proven correctness, but no attempt at adapting their work to a stable merge *sort* apparently existed. This is one such attempt!

(**Note**: an updated and unpublished version of their paper is available [here](http://ak.hanyang.ac.kr/papers/performant-in-place-merging.pdf)!)<br/><br/>

**What separates this from those other in-place merge papers?**<br/>
Dr. Kutzner's and Dr. Kim's paper addresses this, but many of the papers define algorithms that are unstable, impractical (as in too slow to be of general use), or <i>theoretical</i>. Their paper is one of the few to provide a full implementation for a fast and stable in-place merge, and the published performance results were promising.<br/><br/>

**Head-to-head**<br/>
&nbsp;&nbsp;• 80-90% of the speed of libc++'s stable_sort() for highly random input with fewer than ~10 million elements.<br/>
&nbsp;&nbsp;• Starts to *exceed* stable_sort()'s performance for larger arrays (not yet sure why – needs more research).<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(**Update**: Apparently this is caused by virtual memory kicking in on my system, and it's normally slower)<br/>
&nbsp;&nbsp;• Up to 10x faster when the data is already partially sorted or otherwise has a less-than-random distribution.<br/>
&nbsp;&nbsp;• 3-15x faster than inplace_stable_sort(), which is libc++'s equivalent O(1) sort function.<br/>
&nbsp;&nbsp;• Can easily be modified to use any amount of memory you give it – including zero.<br/><br/>

**If you want to know how it works, check out the documentation:**<br/>
&nbsp;&nbsp;• [Chapter 1: Tools](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201.%20Tools.md)<br/>
&nbsp;&nbsp;• [Chapter 2: Merging](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202.%20Merging.md)<br/>
&nbsp;&nbsp;• [Chapter 3: In-Place](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203.%20In-Place.md)<br/>
&nbsp;&nbsp;• [Chapter 4: Faster!](https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%204.%20Faster!.md)<br/><br/>

WikiSort is fast, stable, uses O(1) memory, and **[public domain](https://github.com/BonzaiThePenguin/WikiSort/blob/master/LICENSE)**! It will also change as superior techniques become known – if you see something in the algorithm that could be improved, please flag the issue!<br/><br/>

[C](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c), [C++](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp), and [Java](https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.java) versions are currently available. Enjoy!

**Quick extra note**: If you're wondering what to call this type of sort, the authors suggested "block merge-sort" (possibly "block sort" for short), as it would fit the existing pattern of "heap sort" and "weak heap sort". I guess KimSort is a no-go.
