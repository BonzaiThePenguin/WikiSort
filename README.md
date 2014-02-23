bzSort
======

Hybrid sorting algorithm that's stable, has an O(n) best case and quasilinear worst case, and uses O(1) memory.<br/>
<br/>
<b>Features:</b><br/>
&nbsp;&nbsp;• Does not use recursion or dynamic allocations, so it optimizes/inlines well.<br/>
&nbsp;&nbsp;• Runs faster if the data is already partially sorted.<br/>
&nbsp;&nbsp;• 65-100% faster than <a href="https://github.com/patperry/timsort/blob/master/stresstest.c">Timsort</a> for random data, and exactly as fast in the best case (pre-sorted data).<br/>
&nbsp;&nbsp;• Typically faster than quick sort, while also being stable and having a much better worst-case.<br/>
&nbsp;&nbsp;• <b>Public domain</b>. Do whatever you want with it.<br/>


The initial version is a C #define so it can work with any data type. I apologize for its nasty appearance. If you want a more readable version, bzSort_int.c contains a C function that works with int arrays.<br/><br/>

This is basically just a standard <a href="http://www.algorithmist.com/index.php/Merge_sort#Bottom-up_merge_sort">bottom-up merge sort</a> with the following changes:<br/>

&nbsp;&nbsp;• also has the standard optimization of using insertion sort in the lower levels<br/>
&nbsp;&nbsp;• uses a fixed-size circular buffer for swaps/merges<br/>
&nbsp;&nbsp;• avoids unnecessary merges, which makes it faster for partially-sorted data<br/>
&nbsp;&nbsp;• <b>uses a different method for calculating the ranges to merge (see below)</b><br/>
<br/>
Here's how the bottom-up sort looks for an array of a size that happens to be a power of two:<br/>

    void sort(int a[], uint64 count) {
       uint64 index = 0;
       while (index < count) {
          uint64 merge = index;
          index += 2;
          uint64 iteration = index;
          uint64 length = 1;
          while (is_even(iteration)) {
             uint64 start = merge;
             uint64 mid = merge + length;
             uint64 end = merge + length + length;
             
             printf("merge %llu-%llu and %llu-%llu\n", start, mid - 1, mid, end - 1);
             /* actual sorting code goes here */
             
             length *= 2;
             merge -= length;
             iteration /= 2;
          }
       }
    }

For an array of size 16, it prints this (the operation is shown to the right):

                                       [ 15  2   13  7   3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 0-0 and 1-1                  [[15][2 ] 13  7   3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 2-2 and 3-3                  [ 2   15 [13][7 ] 3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 0-1 and 2-3                  [[2   15][7   13] 3   0   11  4   12  6   10  14  1   9   8   5  ]
    merge 4-4 and 5-5                  [ 2   7   13  15 [3 ][0 ] 11  4   12  6   10  14  1   9   8   5  ]
    merge 6-6 and 7-7                  [ 2   7   13  15  0   3  [11][4 ] 12  6   10  14  1   9   8   5  ]
    merge 4-5 and 6-7                  [ 2   7   13  15 [0   3 ][4   11] 12  6   10  14  1   9   8   5  ]
    merge 0-3 and 4-7                  [[2   7   13  15][0   3   4   11] 12  6   10  14  1   9   8   5  ]
    merge 8-8 and 9-9                  [ 0   2   3   4   7   11  13  15 [12][6 ] 10  14  1   9   8   5  ]
    merge 10-10 and 11-11              [ 0   2   3   4   7   11  13  15  6   12 [10][14] 1   9   8   5  ]
    merge 8-9 and 10-11                [ 0   2   3   4   7   11  13  15 [6   12][10  14] 1   9   8   5  ]
    merge 12-12 and 13-13              [ 0   2   3   4   7   11  13  15  6   10  12  14 [1 ][9 ] 8   5  ]
    merge 14-14 and 15-15              [ 0   2   3   4   7   11  13  15  6   10  12  14  1   9  [8 ][5 ]]
    merge 12-13 and 14-15              [ 0   2   3   4   7   11  13  15  6   10  12  14 [1   9 ][5   8 ]]
    merge 8-11 and 12-15               [ 0   2   3   4   7   11  13  15 [6   10  12  14][1   5   8   9 ]]
    merge 0-7 and 8-15                 [[0   2   3   4   7   11  13  15][1   5   6   8   9   10  12  14 ]
                                       [ 0   1   2   3   4   5   6   7   8   9   10  11  12  13  14  15 ]
Which is of course exactly what we wanted.<br/>
<br/>
<br/>
To extend this logic to non-power-of-two sizes, we simply floor the size down to the nearest power of two for these calculations, then scale back again to get the ranges to merge. Floating-point multiplications are blazing-fast these days so it hardly matters.

    void sort(int a[], uint64 count) {
    >  uint64 pot = floor_power_of_two(count);
    >  double scale = count/(double)pot; // 1.0 <= scale < 2.0
       uint64 index = 0;
       while (index < count) {
          uint64 merge = index; index += 2; uint64 iteration = index, length = 1;
          
          while (is_even(iteration)) {
    >        uint64 start = (merge) * scale;
    >        uint64 mid = (merge + length) * scale;
    >        uint64 end = (merge + length + length) * scale;
             
             printf("merge %llu-%llu and %llu-%llu\n", start, mid - 1, mid, end - 1);
             /* check bzSort_int.c for working code */
             
             length *= 2; merge -= length; iteration /= 2;
          }
       }
    }
    
The multiplication has been proven to be correct for more than 17,179,869,184 elements, which should be adequate. Correctness is defined as (end == count) on the last merge step and enough precision to represent the ranges, as otherwise there would be an off-by-one error due to floating-point inaccuracies. Floats are only precise enough for up to 17 million elements.<br/>

<b>This guarantees that the two ranges being merged will always have the same size to within one item, which makes it more efficient and allows for additional optimizations.</b> From there it was just a matter of implementing a standard merge using a fixed-size circular buffer, using insertion sort for sections that contain 16-31 values (16 * (1.0 <= scale < 2.0)), and adding the special cases.

And by the time all that's added, you end up with something that is apparently faster than any of the other sorting algorithms I've tested on Github thus far, while also being stable and using constant memory. (Testing is performed with arrays from zero to millions of elements, in increments of 2500, and in various configurations.)

<b>This code is public domain, so feel free to use it or contribute in any way you like.</b> Cleaner code, ports, optimizations, more-intelligent special cases, benchmarks on real-world data, it's all welcome.


And, just in case you're completely new to this, type this in the Terminal to compile and run:

    gcc -o bzSort.x bzSort.c
    ./bzSort.x
