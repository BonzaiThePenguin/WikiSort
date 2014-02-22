This algorithm would benefit from the following addition:

#define bzSort(array, compare) ({ \<br/>
&nbsp;&nbsp;&nbsp;__typeof__(array[0]) temp, *bzSort_array = array; const long bzSort_count = array_count; \<br/>
&nbsp;&nbsp;&nbsp;uint64_t i, <b>ascending = 0, descending = 0;</b> \<br/>
&nbsp;&nbsp;&nbsp;<b>for (i = 1; i < bzSort_count; i++) { if (compare(temp, prev) >= 0) ascending++; else descending++; prev = temp; }</b> \<br/>
&nbsp;&nbsp;&nbsp;<b>if (descending > ascending) { reverse(bzSort_array); swap(ascending, descending); }</b> \<br/>
&nbsp;&nbsp;&nbsp;if (bzSort_count < 32) { \<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;/* insertion sort the array */ \<br/>
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[...]<br/><br/>
&nbsp;&nbsp;&nbsp;} else <b>if (descending > 0)</b> { \<br/>

However, a naïve array reverse operation would invalidate the stability of the algorithm:

&nbsp;&nbsp;&nbsp;[5 a] [5 b] [4 c]  <--->  [4 c] [5 b] [5 a]

A more-intelligent reversal is needed before this can be added to the algorithm.
