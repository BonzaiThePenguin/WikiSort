WikiSort
======

WikiSort is a sorting algorithm that's stable, has an O(n) best case and quasilinear worst case, and uses O(1) memory. <b>This is a live standard, and <i>will</i> change as superior techniques become known.</b> Feel free to add your own improvements!<br/>

<br/>
<b>Features:</b><br/>
&nbsp;&nbsp;• Does not use recursion or dynamic allocations, so it optimizes/inlines well.<br/>
&nbsp;&nbsp;• Runs faster if the data is already partially sorted.<br/>
&nbsp;&nbsp;• Is a stable sort, which means equal items retain their order in relation to each other.<br/>
&nbsp;&nbsp;• Despite being an in-place merge, it's highly competitive in speed!<br/>
&nbsp;&nbsp;• <b>Public domain, usable and editable by anyone</b>. Do whatever you want with it.<br/><br/>

Right now WikiSort is basically just a postorder <a href="http://www.algorithmist.com/index.php/Merge_sort#Bottom-up_merge_sort">bottom-up merge sort</a> with the following changes:<br/>

&nbsp;&nbsp;• implements the standard optimization of using insertion sort in the lower levels<br/>
&nbsp;&nbsp;• avoids unnecessary merges, which makes it faster for partially-sorted data<br/>
&nbsp;&nbsp;• calculates the ranges to merge using floating-point math rather than min(range, array_size)<br/>
&nbsp;&nbsp;• uses a simplified and optimized version of an advanced in-place merge algorithm<br/><br/>

<b>A detailed description of how this algorithm works is coming soon!</b> Until then, this algorithm is based on the one described in <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">"Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner</a>. However, it uses a simpler design that allows it to be as fast or <i>faster</i> than the standard merge sort.<br/><br/>

<b>This code is public domain, so feel free to use it or contribute in any way you like.</b> Cleaner code, ports, optimizations, more-intelligent special cases, benchmarks on real-world data, it's all welcome.


And, just in case you're completely new to this, type this in the Terminal to compile and run:

    gcc -o WikiSort.x WikiSort.c
    ./WikiSort.x
