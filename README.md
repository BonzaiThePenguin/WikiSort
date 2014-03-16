WikiSort
======

**Note**: if you want to coordinate your progress with other developers, check out <a href="http://reddit.com/r/BlockMergeSort">/r/BlockMergeSort</a> on Reddit.

WikiSort is a stable bottom-up in-place merge sort based on the work described in <a href="http://ak.hanyang.ac.kr/papers/tamc2008.pdf">"Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner</a> [PDF]. Kim's and Kutzner's algorithm is a stable merge algorithm with great performance characteristics and proven correctness, but no attempt at adapting their work to a stable merge <i>sort</i> apparently existed. This is one such attempt!

<b>What separates this from those other in-place merge papers?</b><br/>
Dr. Kutzner's and Dr. Kim's paper addresses this, but many of the papers define algorithms that are unstable, impractical (as in too slow to be of general use), or <i>theoretical</i>. Their paper is one of the few to provide a full implementation for a fast and stable in-place merge, and the published performance results were promising.

<b>Head-to-head</b><br/>
&nbsp;&nbsp;• 80-90% of the speed of libc++'s stable_sort() for highly random input with fewer than ~10 million elements.<br/>
&nbsp;&nbsp;• Starts to <i>exceed</i> stable_sort()'s performance for larger arrays (not yet sure why – needs more research).<br/>
&nbsp;&nbsp;• Up to 10x faster when the data is already partially sorted or otherwise has a less-than-random distribution.<br/>
&nbsp;&nbsp;• 3-15x faster than inplace_stable_sort(), which is libc++'s equivalent O(1) sort function.

<b>If you want to know how it works, check out the documentation:</b><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Tools.md">Chapter 1: Tools</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md">Chapter 2: Merging</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203:%20In-Place.md">Chapter 3: In-Place</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%204:%20Faster!.md">Chapter 4: Faster!</a><br/><br/>

WikiSort is fast, stable, uses O(1) memory, and <b><a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/LICENSE">public domain</a></b>! It will also change as superior techniques become known – if you see something in the algorithm that could be improved, please flag the issue!<br/><br/>

<a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c">C</a>, <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp">C++</a>, and <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.java">Java</a> versions are currently available. Please note that while it is already highly practical, it is not yet a 100% implementation of their work.<br/><br/>


<b>Quick extra note:</b> If you're wondering what to call this type of sort, the authors suggested "block merge-sort", as it would fit the existing pattern of "heap sort" and "weak heap sort". I guess KimSort is a no-go.
