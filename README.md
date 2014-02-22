bzSort
======

Hybrid sorting algorithm that's stable, has an O(n) best case, quasilinear worst case, and uses O(1) memory.

This is basically just a standard merge sort with the following changes:

  • the standard optimization of using insertion sort in the lower levels
  • a fixed-size circular buffer instead of a dynamically-sized array
  • various special cases for descending data, unnecessary merges, etc.
  • a single uint64 to keep track of the "recursion"

I'm not sure why I can't find more algorithms using this, but since merge sort always splits in the exact middle, it's trivial to reproduce its order of execution using a uint64 rather than a set of dynamically-allocated structures. Here's how it would look for an array of a size that happens to be a power of two:

  void sort(int a[], uint64 count) {
	  uint64 index = 0;
	  while (index < count) {
	  	uint64 merge = index; index += 2; uint64 iteration = index, length = 1;
	  	while (is_even(iteration)) { // ((iteration & 0x1) == 0x0)
	  		uint64 start = merge, mid = merge + length, end = merge + length + length;
	  		printf("merge %llu-%llu and %llu-%llu\n", start, mid - 1, mid, end - 1);
		  	length <<= 1; merge -= length; iteration >>= 1;
      }
	  }
  }

For an array of size 16, it prints this:
  merge 0-0 and 1-1
  merge 2-2 and 3-3
  merge 0-1 and 2-3
  merge 4-4 and 5-5
  merge 6-6 and 7-7
  merge 4-5 and 6-7
  merge 0-3 and 4-7
  merge 8-8 and 9-9
  merge 10-10 and 11-11
  merge 8-9 and 10-11
  merge 12-12 and 13-13
  merge 14-14 and 15-15
  merge 12-13 and 14-15
  merge 8-11 and 12-15
  merge 0-7 and 8-15

Which is of course exactly what we wanted.


To extend this logic to non-power-of-two sizes, we simply floor the size down to the nearest power of two for these calculations, then scale back again to get the ranges to merge. Floating-point multiplications are blazing-fast these days so it hardly matters.

  void sort(int a[], uint64 count) {
	  uint64 pot = bzFloorPowerOfTwo(count);
    double scale = count/(double)pot; // 1.0 <= scale < 2.0
    uint64 index = 0;
	  while (index < count) {
	  	uint64 merge = index; index += 2; uint64 iteration = index, length = 1;
	  	while (is_even(iteration)) { // ((iteration & 0x1) == 0x0)
	  		uint64 start = merge * scale, mid = (merge + length) * scale, end = (merge + length + length) * scale;
	  		printf("merge %llu-%llu and %llu-%llu\n", start, mid - 1, mid, end - 1);
		  	length <<= 1; merge -= length; iteration >>= 1;
      }
	  }
  }

The multiplication has been proven to be correct for more than 17,179,869,184 elements, which should be adequate.


Anyway, from there it was just a matter of implementing a standard merge using a fixed-size circular buffer, using insertion sort for sections that contain 16-31 values (16 * (1.0 <= scale < 2.0)), and adding in the special cases.
