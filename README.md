WikiSort
======

WikiSort is a stable bottom-up in-place merge sort based on the work described in <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">"Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner</a>. Kim's and Kutzner's algorithm is a stable merge algorithm with great performance characteristics and proven correctness, but no attempt at adapting their work to a stable merge <i>sort</i> apparently existed. This is one such attempt!

<b>What separate this from those other in-place merge papers?</b><br/>
Dr. Kutzner's and Dr. Kim's paper addresses this, but many of the papers define algorithms that are unstable, impractical (as in too slow to be of general use), or <i>theoretical</i>. Their paper is one of the few to provide a full implementation for a fast and stable in-place merge, and the published performance results were promising.

<b>If you want to know how it works, check out the documentation:</b><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Tools.md">Chapter 1: Tools</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md">Chapter 2: Merging</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203:%20In-Place.md">Chapter 3: In-Place</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%204:%20Faster!.md">Chapter 4: Faster!</a><br/><br/>

WikiSort is fast, stable, uses O(1) memory, and <b><a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/LICENSE">public domain</a></b>! It is also an <b>open standard</b> and will change as superior techniques become known – if you see something in the algorithm that could be improved, please flag the issue!<br/><br/>

<a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c">C</a>, <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp">C++</a>, and <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.java">Java</a> versions are currently available.<br/><br/>
