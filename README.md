bzSort
======

Hybrid sorting algorithm that's stable, has an O(n) best case and quasilinear worst case, and uses O(1) memory.<br/>
<br/>
This is basically just a standard merge sort with the following changes:<br/>
<br/>
&nbsp;&nbsp;• the standard optimization of using insertion sort in the lower levels<br/>
&nbsp;&nbsp;• a fixed-size circular buffer for swaps instead of a dynamically-sized array<br/>
&nbsp;&nbsp;• avoids unnecessary merges, which makes it faster for partially-sorted data<br/>
&nbsp;&nbsp;• <b>a single uint64 to keep track of the "recursion"</b><br/>
<br/>
I'm not sure why I can't find more algorithms using this, but since merge sort always splits in the exact middle, it's trivial to reproduce its order of execution using a uint64 rather than using recursion or a set of dynamically-allocated structures. Here's how it would look for an array of a size that happens to be a power of two:<br/>
<br/>
void sort(int a[], uint64 count) {<br/>
&nbsp;&nbsp;&nbsp;uint64 index = 0;<br/>
&nbsp;&nbsp;&nbsp;while (index < count) {<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint64 merge = index; index += 2; uint64 iteration = index, length = 1;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;while (is_even(iteration)) { // ((iteration & 0x1) == 0x0)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint64 start = merge, mid = merge + length, end = merge + length + length;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;printf("merge %llu-%llu and %llu-%llu\n", start, mid - 1, mid, end - 1);<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;length <<= 1; merge -= length; iteration >>= 1;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;}<br/>
&nbsp;&nbsp;&nbsp;}<br/>
}<br/>
<br/>
For an array of size 16, it prints this:<br/>
&nbsp;&nbsp;&nbsp;merge 0-0 and 1-1<br/>
&nbsp;&nbsp;&nbsp;merge 2-2 and 3-3<br/>
&nbsp;&nbsp;&nbsp;merge 0-1 and 2-3<br/>
&nbsp;&nbsp;&nbsp;merge 4-4 and 5-5<br/>
&nbsp;&nbsp;&nbsp;merge 6-6 and 7-7<br/>
&nbsp;&nbsp;&nbsp;merge 4-5 and 6-7<br/>
&nbsp;&nbsp;&nbsp;merge 0-3 and 4-7<br/>
&nbsp;&nbsp;&nbsp;merge 8-8 and 9-9<br/>
&nbsp;&nbsp;&nbsp;merge 10-10 and 11-11<br/>
&nbsp;&nbsp;&nbsp;merge 8-9 and 10-11<br/>
&nbsp;&nbsp;&nbsp;merge 12-12 and 13-13<br/>
&nbsp;&nbsp;&nbsp;merge 14-14 and 15-15<br/>
&nbsp;&nbsp;&nbsp;merge 12-13 and 14-15<br/>
&nbsp;&nbsp;&nbsp;merge 8-11 and 12-15<br/>
&nbsp;&nbsp;&nbsp;merge 0-7 and 8-15<br/>
<br/>
Which is of course exactly what we wanted.<br/>
<br/>
<br/>
To extend this logic to non-power-of-two sizes, we simply floor the size down to the nearest power of two for these calculations, then scale back again to get the ranges to merge. Floating-point multiplications are blazing-fast these days so it hardly matters.<br/>
<br/>
void sort(int a[], uint64 count) {<br/>
&nbsp;&nbsp;&nbsp;<b>uint64 pot = floor_power_of_two(count);</b><br/>
&nbsp;&nbsp;&nbsp;<b>double scale = count/(double)pot; // 1.0 <= scale < 2.0</b><br/>
&nbsp;&nbsp;&nbsp;uint64 index = 0;<br/>
&nbsp;&nbsp;&nbsp;while (index < count) {<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint64 merge = index; index += 2; uint64 iteration = index, length = 1;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;while (is_even(iteration)) { // ((iteration & 0x1) == 0x0)<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;uint64 start = merge <b>* scale</b>, mid = (merge + length) <b>* scale</b>, end = (merge + length + length) <b>* scale</b>;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;printf("merge %llu-%llu and %llu-%llu\n", start, mid - 1, mid, end - 1);<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;length <<= 1; merge -= length; iteration >>= 1;<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;}<br/>
&nbsp;&nbsp;&nbsp;}<br/>
}<br/>

The multiplication has been proven to be correct for more than 17,179,869,184 elements, which should be adequate. Correctness is defined as (end == count) on the last merge step, as otherwise there would be an off-by-one error due to floating-point inaccuracies. Floats are only accurate enough for up to 17 million elements.<br/>
<br/>
<br/>
Anyway, from there it was just a matter of implementing a standard merge using a fixed-size circular buffer, using insertion sort for sections that contain 16-31 values (16 * (1.0 <= scale < 2.0)), and adding the special cases.
