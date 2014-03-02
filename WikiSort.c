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
#endif
#define Var(variable_name, variable_value, more...) __typeof__(variable_value, ##more) variable_name = variable_value, ##more
#ifndef Max
	#define Max(x, y) ({ Var(x1, x); Var(y1, y); (x1 > y1) ? x1 : y1; })
#endif
#ifndef Min
	#define Min(x, y) ({ Var(x1, x); Var(y1, y); (x1 < y1) ? x1 : y1; })
#endif


#define Allocate(type, count) (type *)malloc((count) * sizeof(type))
#define Seconds() (clock() * 1.0/CLOCKS_PER_SEC)


// structure to represent ranges within the array
#define MakeRange(/* long */ start, length) ((Range){(long)(start), length})
#define RangeBetween(/* long */ start, end) ({ long RangeStart = (long)(start); MakeRange(RangeStart, (end) - RangeStart); })
#define ZeroRange() MakeRange(0, 0)
typedef struct { long start, length; } Range;


// toolbox functions used by the sorter

// 63 -> 32, 64 -> 64, etc.
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
#define IsEven(value) ((value & 0x1) == 0x0)

// swap value1 and value2
#define Swap(value1, value2) ({ \
	Var(SwapValue1, &(value1)); \
	Var(SwapValue2, &(value2)); \
	Var(SwapValue, *SwapValue1); \
	*SwapValue1 = *SwapValue2; \
	*SwapValue2 = SwapValue; \
})

// reverse a range within the array
#define Reverse(array_instance, range) ({ \
	Var(Reverse_array, array_instance); Range Reverse_range = range; \
	long Reverse_index; \
	for (Reverse_index = Reverse_range.length/2 - 1; Reverse_index >= 0; Reverse_index--) \
		Swap(Reverse_array[Reverse_range.start + Reverse_index], Reverse_array[Reverse_range.start + Reverse_range.length - Reverse_index - 1]); \
})

// swap a sequence of values in an array
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

// rotate the values in an array ([0 1 2 3] becomes [3 0 1 2] if we rotate by -1)
#define Rotate(array, amount, range) ({ \
	Var(Rotate_array, array); long Rotate_amount = amount; Range Rotate_range = range; \
	if (Rotate_range.length != 0 && Rotate_amount != 0) { \
		if (Rotate_amount < 0 && Rotate_amount >= -cache_size) { \
			/* copy the right side of the array to the cache, memmove the rest of it, and copy the right side back to the left side */ \
			Rotate_amount = (-Rotate_amount) % Rotate_range.length; \
			long Rotate_size = Rotate_range.length - Rotate_amount; \
			memcpy(&cache[0], &Rotate_array[Rotate_range.start + Rotate_size], Rotate_amount * sizeof(Rotate_array[0])); \
			memmove(&Rotate_array[Rotate_range.start + Rotate_amount], &Rotate_array[Rotate_range.start], Rotate_size * sizeof(Rotate_array[0])); \
			memcpy(&Rotate_array[Rotate_range.start], &cache[0], Rotate_amount * sizeof(Rotate_array[0])); \
		} else if (Rotate_amount > 0 && Rotate_amount <= cache_size) { \
			Rotate_amount = Rotate_amount % Rotate_range.length; \
			long Rotate_size = Rotate_range.length - Rotate_amount; \
			memcpy(&cache[0], &Rotate_array[Rotate_range.start], Rotate_amount * sizeof(Rotate_array[0])); \
			memmove(&Rotate_array[Rotate_range.start], &Rotate_array[Rotate_range.start + Rotate_amount], Rotate_size * sizeof(Rotate_array[0])); \
			memcpy(&Rotate_array[Rotate_range.start + Rotate_size], &cache[0], Rotate_amount * sizeof(Rotate_array[0])); \
		} else { \
			Range Rotate_range1, Rotate_range2; \
			if (Rotate_amount < 0) { \
				Rotate_amount = (-Rotate_amount) % Rotate_range.length; \
				Rotate_range1 = MakeRange(Rotate_range.start + Rotate_range.length - Rotate_amount, Rotate_amount); \
				Rotate_range2 = MakeRange(Rotate_range.start, Rotate_range.length - Rotate_amount); \
			} else { \
				Rotate_amount = Rotate_amount % Rotate_range.length; \
				Rotate_range1 = MakeRange(Rotate_range.start, Rotate_amount); \
				Rotate_range2 = MakeRange(Rotate_range.start + Rotate_amount, Rotate_range.length - Rotate_amount); \
			} \
			Reverse(Rotate_array, Rotate_range1); \
			Reverse(Rotate_array, Rotate_range2); \
			Reverse(Rotate_array, Rotate_range); \
		} \
	} \
})

// merge operation which uses an internal or external buffer
#define WikiMerge(array, buffer, A, B, compare) ({ \
	Var(Merge_array, array); Var(Merge_buffer, buffer); Var(Merge_A, A); Var(Merge_B, B); \
	if (compare(Merge_array[Merge_A.start + Merge_A.length - 1], Merge_array[Merge_B.start]) > 0) { \
		long A_count = 0, B_count = 0, insert = 0; \
		if (Merge_A.length <= cache_size) { \
			memcpy(&cache[0], &Merge_array[Merge_A.start], Merge_A.length * sizeof(Merge_array[0])); \
			while (A_count < Merge_A.length && B_count < Merge_B.length) \
				Merge_array[Merge_A.start + insert++] = (compare(cache[A_count], Merge_array[Merge_B.start + B_count]) <= 0) ? cache[A_count++] : Merge_array[Merge_B.start + B_count++]; \
			memcpy(&Merge_array[Merge_A.start + insert], &cache[A_count], (Merge_A.length - A_count) * sizeof(Merge_array[0])); \
		} else { \
			BlockSwap(Merge_array, Merge_buffer.start, Merge_A.start, Merge_A.length); \
			while (A_count < A.length && B_count < B.length) { \
				if (compare(Merge_array[Merge_buffer.start + A_count], Merge_array[Merge_B.start + B_count]) <= 0) { \
					Swap(Merge_array[Merge_A.start + insert++], Merge_array[Merge_buffer.start + A_count++]); \
				} else Swap(Merge_array[Merge_A.start + insert++], Merge_array[Merge_B.start + B_count++]); \
			} \
			BlockSwap(Merge_array, Merge_buffer.start + A_count, Merge_A.start + insert, Merge_A.length - A_count); \
		} \
	} \
})

// find the first value within the range that is equal to the value at index
long BinaryInsertFirst(Test array[], long index, Range range, Comparison compare) {
	long min1 = range.start, max1 = range.start + range.length - 1;
	while (min1 < max1) { long mid1 = min1 + (max1 - min1)/2; if (compare(array[mid1], array[index]) < 0) min1 = mid1 + 1; else max1 = mid1; }
	if (min1 == range.start + range.length - 1 && compare(array[min1], array[index]) < 0) min1++;
	return min1;
}

// find the last value within the range that is equal to the value at index. the result is 1 more than the last index
long BinaryInsertLast(Test array[], long index, Range range, Comparison compare) {
	long min1 = range.start, max1 = range.start + range.length - 1;
	while (min1 < max1) { long mid1 = min1 + (max1 - min1)/2; if (compare(array[mid1], array[index]) <= 0) min1 = mid1 + 1; else max1 = mid1; }
	if (min1 == range.start + range.length - 1 && compare(array[min1], array[index]) <= 0) min1++;
	return min1;
}

// n^2 sorting algorithm, used to sort tiny chunks of the full array
void InsertionSort(Test array[], Range range, Comparison compare) {
	long i;
	for (i = range.start + 1; i < range.start + range.length; i++) {
		Test temp = array[i]; long j = i;
		while (j > range.start && compare(array[j - 1], temp) > 0) { array[j] = array[j - 1]; j--; }
		array[j] = temp;
	}
}

// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
void WikiSort(Test array[], const long array_count, Comparison compare) {
	// the various toolbox functions are optimized to take advantage of this cache, so tweak it as desired
	// generally this cache is suitable for arrays of up to size (cache_size^2)
	const long cache_size = 1024;
	Test cache[cache_size];
	
	if (array_count < 32) {
		// insertion sort the entire array, since there are fewer than 32 items
		InsertionSort(array, RangeBetween(0, array_count), compare);
		return;
	}
	
	// calculate how to scale the index value to the range within the array
	long power_of_two = FloorPowerOfTwo(array_count);
	double scale = array_count/(double)power_of_two; // 1.0 <= scale < 2.0
	
	long index, iteration;
	for (index = 0; index < power_of_two; index += 32) {
		// insertion sort from start to mid and mid to end
		long start = index * scale;
		long mid = (index + 16) * scale;
		long end = (index + 32) * scale;
		
		InsertionSort(array, RangeBetween(start, mid), compare);
		InsertionSort(array, RangeBetween(mid, end), compare);
		
		// here's where the fake recursion is handled
		// it's a bottom-up merge sort, but multiplying by scale is more efficient than using Min(end, array_count)
		long merge = index, length = 16;
		for (iteration = index/16 + 2; IsEven(iteration); iteration /= 2) {
			start = merge * scale;
			mid = (merge + length) * scale;
			end = (merge + length + length) * scale;
			
			if (compare(array[start], array[end - 1]) > 0) {
				// the two ranges are in reverse order, so a simple rotation should fix it
				Rotate(array, mid - start, RangeBetween(start, end));
			} else if (compare(array[mid - 1], array[mid]) > 0) {
				// these two ranges weren't already in order, so we'll need to merge them!
				Range A = RangeBetween(start, mid), B = RangeBetween(mid, end);
				
				// if A fits into the cache, perform a simple merge; otherwise perform a trickier in-place merge
				if (A.length <= cache_size) {
					memcpy(&cache[0], &array[A.start], A.length * sizeof(array[0]));
					long A_count = 0, B_count = 0, insert = 0;
					while (A_count < A.length && B_count < B.length) array[A.start + insert++] = (compare(cache[A_count], array[B.start + B_count]) <= 0) ? cache[A_count++] : array[B.start + B_count++];
					memcpy(&array[A.start + insert], &cache[A_count], (A.length - A_count) * sizeof(array[0]));
				} else {
					// try to fill up two buffers with unique values in ascending order
					Range bufferA, bufferB, buffer1, buffer2; long block_size = Max(sqrt(A.length), 2), buffer_size = A.length/block_size;
					for (buffer1.start = A.start + 1, buffer1.length = 1; buffer1.start < A.start + A.length && buffer1.length < buffer_size; buffer1.start++)
						if (compare(array[buffer1.start - 1], array[buffer1.start]) != 0) buffer1.length++;
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
						}
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
						}
					}
					
					if (buffer2.length < buffer_size) {
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
								long min1 = BinaryInsertFirst(array, A.start, RangeBetween(index - block_size, Min(index, B.start + B.length)), compare);
								
								// rotate [A B1] B2 to [B1 A] B2 and recalculate A and B
								Rotate(array, A.length, RangeBetween(A.start, min1));
								A.length--; A.start = min1 - A.length;
								B = RangeBetween(min1, B.start + B.length);
							} else {
								if (B.length <= 0) break;
								long block_size = Max(FloorPowerOfTwo((long)(A.length/(double)B.length)), 1);
								
								// merge B[last] into A
								long index = B.start - block_size;
								while (index >= A.start && compare(array[index], array[B.start + B.length - 1]) >= 0) index -= block_size;
								
								// binary search to find the last index where A[index - 1] <= B[last] < A[index]
								long min1 = BinaryInsertLast(array, B.start + B.length - 1, RangeBetween(Max(index, A.start), index + block_size), compare);
								
								// rotate A1 [A2 B] to A1 [B A2] and recalculate A and B
								Rotate(array, -B.length, RangeBetween(min1, B.start + B.length));
								A = RangeBetween(A.start, min1);
								B = MakeRange(min1, B.length - 1);
							}
						}
						
						return;
					}
					
					// move the unique values to the start of A if needed
					long index, count;
					for (index = bufferA.start - 2, count = 1; count < bufferA.length; index--) {
						if (index == A.start || compare(array[index - 1], array[index]) != 0) {
							Rotate(array, -count, RangeBetween(index + 1, bufferA.start));
							count++; bufferA.start = index + count;
						}
					}
					bufferA.start = A.start;
					
					// move the unique values to the end of B if needed
					for (index = bufferB.start + 1, count = 0; count < bufferB.length; index++) {
						if (index == B.start + B.length || compare(array[index - 1], array[index]) != 0) {
							Rotate(array, count + 1, RangeBetween(bufferB.start, index));
							count++; bufferB.start = index - count;
						}
					}
					bufferB.start = B.start + B.length - bufferB.length;
					
					// break the remainder of A into blocks, which we'll call w. w0 is the uneven-sized first w block
					Range w = RangeBetween(bufferA.start + bufferA.length, A.start + A.length);
					Range w0 = MakeRange(bufferA.start + bufferA.length, w.length % block_size);
					
					// swap the last value of each w block with the value in buffer1
					long w_index;
					for (index = 0, w_index = w0.start + w0.length + block_size - 1; w_index < w.start + w.length; index++, w_index += block_size)
						Swap(array[buffer1.start + index], array[w_index]);
					
					Range last_w = w0, last_v = ZeroRange(), v = MakeRange(B.start, Min(block_size, B.length - bufferB.length));
					w.start += w0.length; w.length -= w0.length;
					w_index = 0;
					long w_min = w.start;
					while (w.length > 0) {
						// if there's a previous v block and the first value of the minimum w block is <= the last value of the previous v block
						if ((last_v.length > 0 && compare(array[w_min], array[last_v.start + last_v.length - 1]) <= 0) || v.length == 0) {
							// figure out where to split the previous v block, and rotate it at the split
							long v_split = BinaryInsertFirst(array, w_min, last_v, compare);
							long v_remaining = last_v.start + last_v.length - v_split;
							
							// swap the minimum w block to the beginning of the rolling w blocks
							BlockSwap(array, w.start, w_min, block_size);
							
							// we need to swap the last item of the previous w block back with its original value, which is stored in buffer1
							// since the w0 block did not have its value swapped out, we need to make sure the previous w block is not unevenly sized
							Swap(array[w.start + block_size - 1], array[buffer1.start + w_index++]);
							
							// now we need to split the previous v block at v_split and insert the minimum w block in-between the two parts, using a rotation
							Rotate(array, v_remaining, RangeBetween(v_split, w.start + block_size));
							
							// locally merge the previous w block with the v blocks that follow it, using the buffer as swap space
							WikiMerge(array, buffer2, last_w, RangeBetween(last_w.start + last_w.length, v_split), compare);
							
							// now we need to update the ranges and stuff
							last_w = MakeRange(w.start - v_remaining, block_size);
							last_v = MakeRange(last_w.start + last_w.length, v_remaining);
							w.start += block_size; w.length -= block_size;
							
							// search the last value of the remaining w blocks to find the new minimum w block (that's why we wrote unique values to them!)
							w_min = w.start + block_size - 1;
							long w_find;
							for (w_find = w_min + block_size; w_find < w.start + w.length; w_find += block_size)
								if (compare(array[w_find], array[w_min]) < 0) w_min = w_find;
							w_min -= (block_size - 1);
						} else if (v.length < block_size) {
							// move the last v block, which is unevenly sized, to before the remaining w blocks, by using a rotation
							Rotate(array, -v.length, RangeBetween(w.start, v.start + v.length));
							last_v = MakeRange(w.start, v.length);
							w.start += v.length; w_min += v.length;
							v.length = 0;
						} else {
							// roll the leftmost w block to the end by swapping it with the next v block
							BlockSwap(array, w.start, v.start, block_size);
							last_v = MakeRange(w.start, block_size);
							if (w_min == w.start) w_min = w.start + w.length;
							w.start += block_size;
							v.start += block_size;
							if (v.start + v.length > bufferB.start) v.length = bufferB.start - v.start;
						}
					}
					
					// merge the last w block with the remaining v blocks
					WikiMerge(array, buffer2, last_w, RangeBetween(last_w.start + last_w.length, B.start + B.length - bufferB.length), compare);
					
					// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
					// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
					InsertionSort(array, buffer2, compare);
					
					// redistribute bufferA back into the array
					for (index = bufferA.start + bufferA.length; bufferA.length > 0; index++) {
						if (index == bufferB.start || compare(array[index], array[bufferA.start]) >= 0) {
							long amount = index - (bufferA.start + bufferA.length);
							Rotate(array, -amount, RangeBetween(bufferA.start, index));
							bufferA.start += amount + 1; bufferA.length--; index--;
						}
					}
					
					// redistribute bufferB back into the array
					for (index = bufferB.start; bufferB.length > 0; index--) {
						if (index == A.start || compare(array[index - 1], array[bufferB.start + bufferB.length - 1]) <= 0) {
							long amount = bufferB.start - index;
							Rotate(array, amount, RangeBetween(index, bufferB.start + bufferB.length));
							bufferB.start -= amount; bufferB.length--; index++;
						}
					}
				}
			}
			
			// the merges get twice as large after each iteration, until eventually we merge the entire array
			length += length; merge -= length;
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
	
	// standard merge operation here (only A is copied to the buffer)
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
	
	srand(/*time(NULL)*/ 10141985);
	
	for (total = 0; total < max_size; total += 2048 * 16) {
		for (index = 0; index < total; index++) {
			Test item; item.index = index;
			
			// uncomment the type of data you want to fill the arrays with
			item.value = rand();
			//item.value = total - index + rand() * 1.0/INT_MAX * 5 - 2.5;
			//item.value = index + rand() * 1.0/INT_MAX * 5 - 2.5;
			//item.value = index;
			//item.value = total - index;
			//item.value = 1000;
			//item.value = (rand() * 1.0/INT_MAX <= 0.9) ? index : (index - 2);
			
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
		assert(compare(array1[0], array2[0]) == 0);
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
