WikiSort
======

There's a common saying regarding sorting algorithms: Fast, stable, and low-memory. <b>Pick two</b>.

<b><a href="http://en.wikipedia.org/wiki/Quicksort">Quicksort</a></b> is fast and has low-memory requirements, but it is not <a href="http://en.wikipedia.org/wiki/Sorting_algorithm#Stability">stable</a>. This means that if a user has items their inventory sorted by name and they go to sort by cost, items with the same cost might not be in alphabetical order anymore.<br/>

<b><a href="http://en.wikipedia.org/wiki/Merge_sort">Merge sort</a></b> is fast and stable, but it requires a second array at least half the size of the original array. This memory usage becomes quite expensive for large data sets, and it isn't always even possible to allocate that extra array.<br/>

<b><a href="http://en.wikipedia.org/wiki/In-place_merge_sort#Variants">In-place merge sort</a></b> is stable and has low-memory requirements, but all of the common variants are <i>incredibly</i> slow. Even GCC's official inplace_stable_sort function is over <b>5 times</b> slower than a standard merge sort!<br/>

<b><a href="http://en.wikipedia.org/wiki/Merge_sort">Insertion sort</a></b> is stable and has low-memory requirements, and it's even very fast for small arrays, but for larger arrays it becomes unbearably slow. This is because it's actually an O(n^2) algorithm, which means for every n items, it has to perform some multiple of n^2 operations. So for 1000 items it's already performing 1 million operations, and for 1 million items it has to perform <b>one trillion</b> operations.<br/>

Other common sorting algorithms include <b><a href="http://en.wikipedia.org/wiki/Heapsort">Heapsort</a></b>, <b><a href="http://en.wikipedia.org/wiki/Smoothsort">Smoothsort</a></b>, and <b><a href="http://en.wikipedia.org/wiki/Timsort">Timsort</a></b>, but they too must sacrifice speed, stability, or memory.<br/>

===========================
That's where <b>WikiSort</b> comes in. WikiSort is stable, it uses O(1) memory, and at its very <i>worst</i> it still matches 85% of the speed of a standard merge sort! And when the data is already partially sorted – a common situation – it easily pulls ahead of that too.<br/><br/>

<b>How does it work?</b><br/>
WikiSort is an in-place merge sort at heart, but it bears little resemblance to the existing algorithms. Its in-place merge is based on the work described in <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">"Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner</a>, although it uses a highly simplified and optimized design.

<b>If you want to learn more, check out the documentation!</b><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Tools.md">Chapter 1: Tools</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md">Chapter 2: Merging</a><br/>
&nbsp;&nbsp;• <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%203:%20In-Place.md">Chapter 3: In-Place</a><br/><br/>

Fast, stable, and low-memory. And best of all, it's <b>public domain</b>! Feel free to use it or contribute in any way you like – that's the <i>Wiki</i> in WikiSort.<br/><br/>

<a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c">C</a> and <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp">C++</a> versions are currently available. Enjoy!<br/><br/>
