#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <limits.h>


// structure to test stable sorting (index will contain its original index in the array, to make sure it doesn't switch places with other items)
typedef struct { int value, index; } Test;
int Compare(Test item1, Test item2) {
	if (item1.value < item2.value) return -1;
	if (item1.value > item2.value) return 1;
	return 0;
}
typedef int (*Comparison)(Test, Test);



// various #defines for the C code
#ifndef true
	#define true 1
	#define false 0
#endif
#define Var(name, value...) __typeof__(value) name = value
#define Max(x, y) ({ Var(x1, x); Var(y1, y); (x1 > y1) ? x1 : y1; })
#define Min(x, y) ({ Var(x1, x); Var(y1, y); (x1 < y1) ? x1 : y1; })

#define Allocate(type, count) (type *)malloc((count) * sizeof(type))
#define Seconds() (clock() * 1.0/CLOCKS_PER_SEC)


// structure to represent ranges within the array
#define MakeRange(start, length) ((Range){(long)(start), length})
#define RangeBetween(start, end) ({ long RangeStart = (long)(start); MakeRange(RangeStart, (end) - RangeStart); })
#define ZeroRange() MakeRange(0, 0)
typedef struct { long start, length; } Range;


// toolbox functions used by the sorter

// 63 -> 32, 64 -> 64, etc.
// if you want to use this outside of the sort function for general use,
// you should probably switch this over to uint64_t
long FloorPowerOfTwo(long x) {
	x |= (x >> 1); x |= (x >> 2); x |= (x >> 4);
	x |= (x >> 8); x |= (x >> 16); x |= (x >> 32);
	x -= (x >> 1) & 0x5555555555555555;
	x = (x & 0x3333333333333333) + ((x >> 2) & 0x3333333333333333);
	x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0F;
	x += x >> 8; x += x >> 16; x += x >> 32;
	x &= 0x7F; return (x == 0) ? 0 : (1 << (x - 1));
}

// if your language does not support bitwise operations for some reason, you can use (floor(value/2) * 2 == value)
#define IsEven(value) (((value) & 0x1) == 0x0)

// swap value1 and value2
#define Swap(value1, value2) { \
	Var(SwapValue1, &(value1)); \
	Var(SwapValue2, &(value2)); \
	Var(SwapValue, *SwapValue1); \
	*SwapValue1 = *SwapValue2; \
	*SwapValue2 = SwapValue; \
}

// reverse a range within the array
#define Reverse(array, range) { \
	Var(Reverse_array, array); Range Reverse_range = range; \
	long Reverse_index; \
	for (Reverse_index = Reverse_range.length/2 - 1; Reverse_index >= 0; Reverse_index--) \
		Swap(Reverse_array[Reverse_range.start + Reverse_index], Reverse_array[Reverse_range.start + Reverse_range.length - Reverse_index - 1]); \
}

// find the index of the first value within the range that is equal to array[index]
long BinaryFirst(Test array[], long index, Range range, Comparison compare) {
	long start = range.start, end = range.start + range.length - 1;
	while (start < end) { long mid = start + (end - start)/2; if (compare(array[mid], array[index]) < 0) start = mid + 1; else end = mid; }
	if (start == range.start + range.length - 1 && compare(array[start], array[index]) < 0) start++;
	return start;
}

// find the index of the last value within the range that is equal to array[index], plus 1
long BinaryLast(Test array[], long index, Range range, Comparison compare) {
	long start = range.start, end = range.start + range.length - 1;
	while (start < end) { long mid = start + (end - start)/2; if (compare(array[mid], array[index]) <= 0) start = mid + 1; else end = mid; }
	if (start == range.start + range.length - 1 && compare(array[start], array[index]) <= 0) start++;
	return start;
}

// n^2 sorting algorithm used to sort tiny chunks of the full array
void InsertionSort(Test array[], Range range, Comparison compare) {
	long i, j;
	for (i = range.start + 1; i < range.start + range.length; i++) {
		Test temp = array[i];
		for (j = i; j > range.start && compare(array[j - 1], temp) > 0; j--) array[j] = array[j - 1];
		array[j] = temp;
	}
}

// swap a series of values in the array
#define BlockSwap(array, start1, start2, block_size) ({ \
	Var(Swap_array, array); Var(Swap_start1, start1); Var(Swap_start2, start2); Var(Swap_size, block_size); \
	if (Swap_size <= cache_size) { \
		memcpy(&cache[0], &Swap_array[Swap_start1], Swap_size * sizeof(Swap_array[0])); \
		memmove(&Swap_array[Swap_start1], &Swap_array[Swap_start2], Swap_size * sizeof(Swap_array[0])); \
		memcpy(&Swap_array[Swap_start2], &cache[0], Swap_size * sizeof(Swap_array[0])); \
	} else { \
		Var(Swap_array, array); Var(Swap_start1, start1); Var(Swap_start2, start2); Var(Swap_size, block_size); \
		long Swap_index; \
		for (Swap_index = 0; Swap_index < Swap_size; Swap_index++) Swap(Swap_array[Swap_start1 + Swap_index], Swap_array[Swap_start2 + Swap_index]); \
	} \
})

// rotate the values in an array ([0 1 2 3] becomes [3 0 1 2] if we rotate by +1)
#define Rotate(array, amount, range) ({ \
	Var(Rotate_array, array); long Rotate_amount = amount; Range Rotate_range = range; \
	if (Rotate_range.length != 0) { \
		long Rotate_split, Rotate_move, Rotate_copy; \
		if (Rotate_amount < 0) Rotate_split = Rotate_amount = (-Rotate_amount) % Rotate_range.length; \
		else { Rotate_amount = Rotate_amount % Rotate_range.length; Rotate_split = Rotate_range.length - Rotate_amount; } \
		if (Rotate_amount != 0) { \
			/* Rotate_range1 will be the smaller of the two ranges */ \
			Range Rotate_range1 = MakeRange(Rotate_range.start, Rotate_split); \
			Range Rotate_range2 = MakeRange(Rotate_range.start + Rotate_split, Rotate_range.length - Rotate_split); \
			if (Rotate_range2.length < Rotate_range1.length) { \
				Rotate_move = Rotate_range2.start + Rotate_range2.length - Rotate_range1.length; \
				Rotate_copy = Rotate_range1.start; \
				Swap(Rotate_range1, Rotate_range2); \
			} else { \
				Rotate_move = Rotate_range1.start; \
				Rotate_copy = Rotate_range1.start + Rotate_range2.length; \
			} \
			/* when range1 has length 1 it should use a local variable rather than using memcpy */ \
			/* when range1 has length <= cache_size it should memcpy to the cache rather than reversing */ \
			/* otherwise it should perform a standard rotation by using three reverse calls */ \
			if (Rotate_range1.length == 1) { \
				Var(Rotate_value, Rotate_array[Rotate_range1.start]); \
				memmove(&Rotate_array[Rotate_move], &Rotate_array[Rotate_range2.start], Rotate_range2.length * sizeof(Rotate_array[0])); \
				Rotate_array[Rotate_copy] = Rotate_value; \
			} else if (Rotate_range1.length <= cache_size) { \
				memcpy(&cache[0], &Rotate_array[Rotate_range1.start], Rotate_range1.length * sizeof(Rotate_array[0])); \
				memmove(&Rotate_array[Rotate_move], &Rotate_array[Rotate_range2.start], Rotate_range2.length * sizeof(Rotate_array[0])); \
				memcpy(&Rotate_array[Rotate_copy], &cache[0], Rotate_range1.length * sizeof(Rotate_array[0])); \
			} else { \
				Reverse(Rotate_array, Rotate_range); \
				Reverse(Rotate_array, Rotate_range1); \
				Reverse(Rotate_array, Rotate_range2); \
			} \
		} \
	} \
})

// merge operation which uses an internal or external buffer
#define WikiMerge(array, buffer, A, B) ({ \
	Var(Merge_array, array); Var(Merge_buffer, buffer); Var(Merge_A, A); Var(Merge_B, B); \
	if (Merge_B.length > 0 && compare(Merge_array[Merge_A.start + Merge_A.length - 1], Merge_array[Merge_B.start]) > 0) { \
		/* find the part where B will first be inserted into A, as everything before that point is already sorted */ \
		Merge_A = RangeBetween(BinaryLast(Merge_array, Merge_B.start, Merge_A, compare), Merge_A.start + Merge_A.length); \
		long A_count = 0, B_count = 0, insert = 0; \
		if (Merge_A.length <= cache_size) { \
			memcpy(&cache[0], &Merge_array[Merge_A.start], Merge_A.length * sizeof(Merge_array[0])); \
			while (true) { \
				if (compare(cache[A_count], Merge_array[Merge_B.start + B_count]) <= 0) { \
					Merge_array[Merge_A.start + insert++] = cache[A_count++]; \
					if (A_count >= Merge_A.length) break; \
				} else { \
					Merge_array[Merge_A.start + insert++] = Merge_array[Merge_B.start + B_count++]; \
					if (B_count >= Merge_B.length) break; \
				} \
			} \
			memcpy(&Merge_array[Merge_A.start + insert], &cache[A_count], (Merge_A.length - A_count) * sizeof(Merge_array[0])); \
		} else { \
			assert(Merge_A.length <= Merge_buffer.length); \
			BlockSwap(Merge_array, Merge_buffer.start, Merge_A.start, Merge_A.length); \
			while (true) { \
				if (compare(Merge_array[Merge_buffer.start + A_count], Merge_array[Merge_B.start + B_count]) <= 0) { \
					Swap(Merge_array[Merge_A.start + insert++], Merge_array[Merge_buffer.start + A_count++]); \
					if (A_count >= Merge_A.length) break; \
				} else { \
					Swap(Merge_array[Merge_A.start + insert++], Merge_array[Merge_B.start + B_count++]); \
					if (B_count >= Merge_B.length) break; \
				} \
			} \
			BlockSwap(Merge_array, Merge_buffer.start + A_count, Merge_A.start + insert, Merge_A.length - A_count); \
		} \
	} \
})

// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
void WikiSort(Test array[], const long array_count, Comparison compare) {
	// reverse ranges of purely descending values
	Range reverse = ZeroRange();
	long index, counter;
	for (index = 1; index < array_count; index++) {
		if (compare(array[index], array[index - 1]) < 0) reverse.length++;
		else { Reverse(array, reverse); reverse = MakeRange(index, 0); }
	}
	Reverse(array, reverse);
	
	// the various toolbox functions are optimized to take advantage of this cache, so tweak it as desired
	// generally this cache is suitable for arrays of up to size (cache_size^2)
	const long cache_size = 1024;
	Test cache[cache_size];
	
	if (array_count < 32) {
		// insertion sort the array, as there are fewer than 32 items
		InsertionSort(array, RangeBetween(0, array_count), compare);
		return;
	}
	
	// calculate how to scale the index value to the range within the array
	const long power_of_two = FloorPowerOfTwo(array_count);
	double scale = array_count/(double)power_of_two; // 1.0 <= scale < 2.0
	
	for (counter = 0; counter < power_of_two; counter += 32) {
		// insertion sort from start to mid and mid to end
		long start = counter * scale;
		long mid = (counter + 16) * scale;
		long end = (counter + 32) * scale;
		
		InsertionSort(array, RangeBetween(start, mid), compare);
		InsertionSort(array, RangeBetween(mid, end), compare);
		
		// here's where the fake recursion is handled
		// it's a bottom-up merge sort, but multiplying by scale is more efficient than using min(end, array_count)
		// the merges get twice as large after each iteration, until eventually we merge the entire array
		long count, minA, indexA, findA;
		long iteration, merge = counter, length = 16;
		for (iteration = counter/16 + 2; IsEven(iteration); iteration /= 2, length += length, merge -= length) {
			start = merge * scale;
			mid = (merge + length) * scale;
			end = (merge + length + length) * scale;
			
			if (compare(array[start], array[end - 1]) > 0) {
				// the two ranges are in reverse order, so a simple rotation should fix it
				Rotate(array, start - mid, RangeBetween(start, end));
			} else if (compare(array[mid - 1], array[mid]) > 0) {
				// these two ranges weren't already in order, so we'll need to merge them!
				Range A = RangeBetween(start, mid), B = RangeBetween(mid, end);
				
				// find the first place in A where B will be inserted, since everything before that point is already in order
				A = RangeBetween(BinaryLast(array, B.start, A, compare), A.start + A.length);
				
				// if A fits into the cache, perform a simple merge; otherwise perform a trickier in-place merge
				if (A.length <= cache_size) {
					WikiMerge(array, ZeroRange(), A, B);
					continue;
				}
				
				// try to fill up two buffers with unique values in ascending order
				Range bufferA, bufferB, buffer1, buffer2; long block_size = Max(sqrt(A.length), 3), buffer_size = A.length/block_size;
				for (buffer1.start = A.start + 1, buffer1.length = 1; buffer1.start < A.start + A.length && buffer1.length < buffer_size; buffer1.start++)
					if (compare(array[buffer1.start - 1], array[buffer1.start]) != 0) buffer1.length++;
				
				// if the size of each block fits into the cache, we only need one buffer for tagging the A blocks
				// this is because the other buffer is used as a swap space for merging the A blocks into the B values that follow it,
				// but we can just use the cache as the buffer instead. this skips some memmoves and an insertion sort
				if (block_size <= cache_size) {
					bufferB = MakeRange(B.start + B.length, 0);
					buffer2 = MakeRange(A.start, 0);
					
					if (buffer1.length < buffer_size) {
						// we couldn't find enough unique values for the buffer in A, so try B
						bufferA = MakeRange(A.start, 0);
						for (buffer1.start = B.start + B.length - 1, buffer1.length = 1; buffer1.start >= B.start && buffer1.length < buffer_size; buffer1.start--)
							if (buffer1.start == B.start || compare(array[buffer1.start - 1], array[buffer1.start]) != 0) buffer1.length++;
						if (buffer1.length == buffer_size) {
							bufferB = MakeRange(buffer1.start, buffer_size);
							buffer1 = MakeRange(B.start + B.length - buffer_size, buffer_size);
							buffer2 = MakeRange(buffer1.start, 0);
						} else buffer1.length = 0; // failure
					} else {
						bufferA = MakeRange(buffer1.start, buffer_size);
						buffer1 = MakeRange(A.start, buffer_size);
					}
				} else {
					for (buffer2.start = buffer1.start, buffer2.length = 0; buffer2.start < A.start + A.length && buffer2.length < buffer_size; buffer2.start++)
						if (compare(array[buffer2.start - 1], array[buffer2.start]) != 0) buffer2.length++;
					
					if (buffer2.length == buffer_size) {
						// we found enough values for both buffers in A
						bufferA = MakeRange(buffer2.start, buffer_size * 2);
						bufferB = MakeRange(B.start + B.length, 0);
						buffer1 = MakeRange(A.start, buffer_size);
						buffer2 = MakeRange(A.start + buffer_size, buffer_size);
					} else if (buffer1.length == buffer_size) {
						// we found enough values for one buffer in A, so we'll need to find one buffer in B
						bufferA = MakeRange(buffer1.start, buffer_size);
						buffer1 = MakeRange(A.start, buffer_size);
						
						for (buffer2.start = B.start + B.length - 1, buffer2.length = 1; buffer2.start >= B.start && buffer2.length < buffer_size; buffer2.start--)
							if (buffer2.start == B.start || compare(array[buffer2.start - 1], array[buffer2.start]) != 0) buffer2.length++;
						if (buffer2.length == buffer_size) {
							bufferB = MakeRange(buffer2.start, buffer_size);
							buffer2 = MakeRange(B.start + B.length - buffer_size, buffer_size);
						} else buffer1.length = 0; // failure
					} else {
						// we were unable to find a single buffer in A, so we'll need to find two buffers in B
						for (buffer1.start = B.start + B.length - 1, buffer1.length = 1; buffer1.start >= B.start && buffer1.length < buffer_size; buffer1.start--)
							if (buffer1.start == B.start || compare(array[buffer1.start - 1], array[buffer1.start]) != 0) buffer1.length++;
						for (buffer2.start = buffer1.start - 1, buffer2.length = 1; buffer2.start >= B.start && buffer2.length < buffer_size; buffer2.start--)
							if (buffer2.start == B.start || compare(array[buffer2.start - 1], array[buffer2.start]) != 0) buffer2.length++;
						
						if (buffer2.length == buffer_size) {
							bufferA = MakeRange(A.start, 0);
							bufferB = MakeRange(buffer2.start, buffer_size * 2);
							buffer1 = MakeRange(B.start + B.length - buffer_size, buffer_size);
							buffer2 = MakeRange(buffer1.start - buffer_size, buffer_size);
						} else buffer1.length = 0; // failure
					}
				}
				
				if (buffer1.length < buffer_size) {
					// we failed to fill both buffers with unique values, which implies we're merging two subarrays with a lot of the same values repeated
					// we can use this knowledge to write a merge operation that is optimized for arrays of repeating values
					
					// this is the rotation-based variant of the Hwang-Lin merge algorithm
					while (true) {
						if (A.length <= B.length) {
							if (A.length <= 0) break;
							long block_size = Max(FloorPowerOfTwo((long)(B.length/(double)A.length)), 1);
							
							// merge A[first] into B
							long index = B.start + block_size;
							while (index < B.start + B.length && compare(array[index], array[A.start]) < 0) index += block_size;
							
							// binary search to find the first index where B[index - 1] < A[first] <= B[index]
							long min1 = BinaryFirst(array, A.start, RangeBetween(index - block_size, Min(index, B.start + B.length)), compare);
							
							// rotate [A B1] B2 to [B1 A] B2 and recalculate A and B
							Rotate(array, -A.length, RangeBetween(A.start, min1));
							A.length--; A.start = min1 - A.length;
							B = RangeBetween(min1, B.start + B.length);
						} else {
							if (B.length <= 0) break;
							long block_size = Max(FloorPowerOfTwo((long)(A.length/(double)B.length)), 1);
							
							// merge B[last] into A
							long index = B.start - block_size;
							while (index >= A.start && compare(array[index], array[B.start + B.length - 1]) >= 0) index -= block_size;
							
							// binary search to find the last index where A[index - 1] <= B[last] < A[index]
							long min1 = BinaryLast(array, B.start + B.length - 1, RangeBetween(Max(index, A.start), index + block_size), compare);
							
							// rotate A1 [A2 B] to A1 [B A2] and recalculate A and B
							Rotate(array, B.length, RangeBetween(min1, B.start + B.length));
							A = RangeBetween(A.start, min1);
							B = MakeRange(min1, B.length - 1);
						}
					}
					
					continue;
				}
				
				// move the unique values to the start of A if needed
				if (bufferA.start > A.start + bufferA.length) for (index = bufferA.start - 2, count = 1; count < bufferA.length; index--) {
					if (index == A.start || compare(array[index - 1], array[index]) != 0) {
						Rotate(array, count, RangeBetween(index + 1, bufferA.start));
						count++; bufferA.start = index + count;
					}
				}
				bufferA.start = A.start;
				
				// move the unique values to the end of B if needed
				if (bufferB.start < B.start + B.length - bufferB.length) for (index = bufferB.start + 1, count = 0; count < bufferB.length; index++) {
					if (index == B.start + B.length || compare(array[index - 1], array[index]) != 0) {
						Rotate(array, -(count + 1), RangeBetween(bufferB.start, index));
						count++; bufferB.start = index - count;
					}
				}
				bufferB.start = B.start + B.length - bufferB.length;
				
				// break the remainder of A into blocks. firstA is the uneven-sized first A block
				Range blockA = RangeBetween(bufferA.start + bufferA.length, A.start + A.length);
				Range firstA = MakeRange(bufferA.start + bufferA.length, blockA.length % block_size);
				
				// swap the second value of each A block with the value in buffer1
				for (index = 0, indexA = firstA.start + firstA.length + 1; indexA < blockA.start + blockA.length; index++, indexA += block_size) Swap(array[buffer1.start + index], array[indexA]);
				
				Range lastA = firstA, lastB = ZeroRange(), blockB = MakeRange(B.start, Min(block_size, B.length - bufferB.length));
				blockA.start += firstA.length; blockA.length -= firstA.length;
				minA = blockA.start; indexA = 0;
				while (true) {
					// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block
					if ((lastB.length > 0 && compare(array[minA], array[lastB.start + lastB.length - 1]) <= 0) || blockB.length == 0) {
						// figure out where to split the previous B block, and rotate it at the split
						long B_split = BinaryFirst(array, minA, lastB, compare);
						long B_remaining = lastB.start + lastB.length - B_split;
						
						// swap the minimum A block to the beginning of the rolling A blocks
						BlockSwap(array, blockA.start, minA, block_size);
						
						// we need to swap the second item of the previous A block back with its original value, which is stored in buffer1
						// since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized
						Swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
						
						// now we need to split the previous B block at B_split and insert the minimum A block in-between the two parts, using a rotation
						Rotate(array, -B_remaining, RangeBetween(B_split, blockA.start + block_size));
						
						// locally merge the previous A block with the B values that follow it, using the buffer as swap space
						WikiMerge(array, buffer2, lastA, RangeBetween(lastA.start + lastA.length, B_split));
						
						// now we need to update the ranges and stuff
						lastA = MakeRange(blockA.start - B_remaining, block_size);
						lastB = MakeRange(lastA.start + lastA.length, B_remaining);
						blockA.start += block_size; blockA.length -= block_size;
						if (blockA.length == 0) break;
						
						// search the last value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!)
						minA = blockA.start + 1;
						for	(findA = minA + block_size; findA < blockA.start + blockA.length; findA += block_size) if (compare(array[findA], array[minA]) < 0) minA = findA;
						minA--;
					} else if (blockB.length < block_size) {
						// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
						Rotate(array, blockB.length, RangeBetween(blockA.start, blockB.start + blockB.length));
						lastB = MakeRange(blockA.start, blockB.length);
						blockA.start += blockB.length; minA += blockB.length;
						blockB.length = 0;
					} else {
						// roll the leftmost A block to the end by swapping it with the next B block
						BlockSwap(array, blockA.start, blockB.start, block_size);
						lastB = MakeRange(blockA.start, block_size);
						if (minA == blockA.start) minA = blockA.start + blockA.length;
						blockA.start += block_size;
						blockB.start += block_size;
						if (blockB.start + blockB.length > bufferB.start) blockB.length = bufferB.start - blockB.start;
					}
				}
				
				// merge the last A block with the remaining B blocks
				WikiMerge(array, buffer2, lastA, RangeBetween(lastA.start + lastA.length, B.start + B.length - bufferB.length));
				
				// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
				// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
				InsertionSort(array, buffer2, compare);
				
				// redistribute bufferA back into the array
				for (index = bufferA.start + bufferA.length; bufferA.length > 0; index++) {
					if (index == bufferB.start || compare(array[index], array[bufferA.start]) >= 0) {
						long amount = index - (bufferA.start + bufferA.length);
						Rotate(array, amount, RangeBetween(bufferA.start, index));
						bufferA.start += amount + 1; bufferA.length--; index--;
					}
				}
				
				// redistribute bufferB back into the array
				for (index = bufferB.start; bufferB.length > 0; index--) {
					if (index == A.start || compare(array[index - 1], array[bufferB.start + bufferB.length - 1]) <= 0) {
						long amount = bufferB.start - index;
						Rotate(array, -amount, RangeBetween(index, bufferB.start + bufferB.length));
						bufferB.start -= amount; bufferB.length--; index++;
					}
				}
			}
		}
	}
}

// standard merge sort
void MergeSortR(Test array[], Range range, Comparison compare, Test buffer[]) {
	if (range.length < 32) {
		InsertionSort(array, range, compare);
		return;
	}
	
	long mid = range.start + range.length/2;
	Range A = RangeBetween(range.start, mid), B = RangeBetween(mid, range.start + range.length);
	
	MergeSortR(array, A, compare, buffer);
	MergeSortR(array, B, compare, buffer);
	
	// standard merge operation here (only A is copied to the buffer, and only the parts that weren't already where they should be)
	A = RangeBetween(BinaryLast(array, B.start, A, compare), A.start + A.length);
	memcpy(&buffer[0], &array[A.start], A.length * sizeof(array[0]));
	long A_count = 0, B_count = 0, insert = 0;
	while (A_count < A.length && B_count < B.length) {
		if (compare(buffer[A_count], array[A.start + A.length + B_count]) <= 0) {
			array[A.start + insert++] = buffer[A_count++];
		} else {
			array[A.start + insert++] = array[A.start + A.length + B_count++];
		}
	}
	memcpy(&array[A.start + insert], &buffer[A_count], (A.length - A_count) * sizeof(array[0]));
}

void MergeSort(Test array[], const long array_count, Comparison compare) {
	Var(buffer, Allocate(Test, array_count));
	MergeSortR(array, MakeRange(0, array_count), compare, buffer);
	free(buffer);
}

int main(int argc, char argv[]) {
	long total, index;
	const long max_size = 1500000;
	Var(array1, Allocate(Test, max_size));
	Var(array2, Allocate(Test, max_size));
	Comparison compare = Compare;
	
	// initialize the random-number generator
	srand(/*time(NULL)*/ 10141985);
	
	for (total = 0; total < max_size; total += 2048 * 16) {
		for (index = 0; index < total; index++) {
			Test item; item.index = index;
			
			// uncomment the type of data you want to fill the arrays with
			item.value = rand();
			//item.value = total - index + rand() * 1.0/RAND_MAX * 5 - 2.5;
			//item.value = index + rand() * 1.0/RAND_MAX * 5 - 2.5;
			//item.value = index;
			//item.value = total - index;
			//item.value = 1000;
			//item.value = (rand() * 1.0/RAND_MAX <= 0.9) ? index : (index - 2);
			//item.value = 1000 + rand() * 1.0/RAND_MAX * 4;
			
			array1[index] = array2[index] = item;
		}
		
		double time1 = Seconds();
		WikiSort(array1, total, compare);
		time1 = Seconds() - time1;
		
		double time2 = Seconds();
		MergeSort(array2, total, compare);
		time2 = Seconds() - time2;
		
		printf("[%ld] wiki: %f, merge: %f (%f%%)\n", total, time1, time2, time2/time1 * 100.0);
		
		// make sure the arrays are sorted correctly, and that the results were stable
		printf("verifying... ");
		fflush(stdout);
		if (total > 0) assert(compare(array1[0], array2[0]) == 0);
		for (index = 1; index < total; index++) {
			assert(compare(array1[index], array2[index]) == 0);
			assert(compare(array1[index], array1[index - 1]) > 0 ||
				   (compare(array1[index], array1[index - 1]) == 0 && array1[index].index > array1[index - 1].index));
		}
		printf("correct!\n");
	}
	
	free(array1); free(array2);
	return 0;
}
