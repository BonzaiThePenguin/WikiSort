/*
 clang -o WikiSort.x WikiSort.c -O3
 (or replace 'clang' with 'gcc')
 ./WikiSort.x
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <limits.h>

/* if true, Verify() will be called after each merge step to make sure it worked correctly */
#define VERIFY false
#define PROFILE false

double reverse_time, insertion_time, scale_time, rotate_time, merge_time2, merge_time3, min_time, pull_out_time, merge_time, insertion_time2, redistribute_time;
double Seconds() { return clock() * 1.0/CLOCKS_PER_SEC; }


/* various #defines for the C code */
#ifndef true
	#define true 1
	#define false 0
	typedef uint8_t bool;
#endif

#define Var(name, value)				__typeof__(value) name = value
#define Allocate(type, count)				(type *)malloc((count) * sizeof(type))

long Min(const long a, const long b) {
	if (a < b) return a;
	return b;
}

long Max(const long a, const long b) {
	if (a > b) return a;
	return b;
}


/* structure to test stable sorting (index will contain its original index in the array, to make sure it doesn't switch places with other items) */
typedef struct {
	int value;
	int index;
} Test;

bool TestCompare(Test item1, Test item2) { return (item1.value < item2.value); }

typedef bool (*Comparison)(Test, Test);



/* structure to represent ranges within the array */
typedef struct {
	long start;
	long end;
} Range;

long Range_length(Range range) { return range.end - range.start; }

Range MakeRange(const long start, const long end) {
	Range range;
	range.start = start;
	range.end = end;
	return range;
}

/* toolbox functions used by the sorter */

/* swap value1 and value2 */
#define Swap(value1, value2) { \
	Var(a, &(value1)); \
	Var(b, &(value2)); \
	\
	Var(c, *a); \
	*a = *b; \
	*b = c; \
}

/* 63 -> 32, 64 -> 64, etc. */
/* apparently this comes from Hacker's Delight? */
long FloorPowerOfTwo (const long value) {
	long x = value;
	x = x | (x >> 1);
	x = x | (x >> 2);
	x = x | (x >> 4);
	x = x | (x >> 8);
	x = x | (x >> 16);
#if __LP64__
	x = x | (x >> 32);
#endif
	return x - (x >> 1);
}

/* find the index of the first value within the range that is equal to array[index] */
long BinaryFirst(const Test array[], const long index, const Range range, const Comparison compare) {
	long start = range.start, end = range.end - 1;
	while (start < end) {
		long mid = start + (end - start)/2;
		if (compare(array[mid], array[index]))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.end - 1 && compare(array[start], array[index])) start++;
	return start;
}

/* find the index of the last value within the range that is equal to array[index], plus 1 */
long BinaryLast(const Test array[], const long index, const Range range, const Comparison compare) {
	long start = range.start, end = range.end - 1;
	while (start < end) {
		long mid = start + (end - start)/2;
		if (!compare(array[index], array[mid]))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.end - 1 && !compare(array[index], array[start])) start++;
	return start;
}

/* n^2 sorting algorithm used to sort tiny chunks of the full array */
void InsertionSort(Test array[], const Range range, const Comparison compare) {
	for (long i = range.start + 1; i < range.end; i++) {
		const Test temp = array[i]; long j;
		for (j = i; j > range.start && compare(temp, array[j - 1]); j--)
			array[j] = array[j - 1];
		array[j] = temp;
	}
}

/* reverse a range within the array */
void Reverse(Test array[], const Range range) {
	for (long index = Range_length(range)/2 - 1; index >= 0; index--)
		Swap(array[range.start + index], array[range.end - index - 1]);
}

/* swap a series of values in the array */
void BlockSwap(Test array[], const long start1, const long start2, const long block_size) {
	for (long index = 0; index < block_size; index++)
		Swap(array[start1 + index], array[start2 + index]);
}

/* rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1) */
void Rotate(Test array[], const long amount, const Range range, Test cache[], const long cache_size) {
	if (Range_length(range) == 0) return;
	
	long split;
	if (amount >= 0) split = range.start + amount;
	else split = range.end + amount;
	
	Range range1 = MakeRange(range.start, split);
	Range range2 = MakeRange(split, range.end);
	
	/* if the smaller of the two ranges fits into the cache, it's *slightly* faster copying it there and shifting the elements over */
	if (Range_length(range1) <= Range_length(range2)) {
		if (Range_length(range1) <= cache_size) {
			memcpy(&cache[0], &array[range1.start], Range_length(range1) * sizeof(array[0]));
			memmove(&array[range1.start], &array[range2.start], Range_length(range2) * sizeof(array[0]));
			memcpy(&array[range1.start + Range_length(range2)], &cache[0], Range_length(range1) * sizeof(array[0]));
			return;
		}
	} else {
		if (Range_length(range2) <= cache_size) {
			memcpy(&cache[0], &array[range2.start], Range_length(range2) * sizeof(array[0]));
			memmove(&array[range2.end - Range_length(range1)], &array[range1.start], Range_length(range1) * sizeof(array[0]));
			memcpy(&array[range1.start], &cache[0], Range_length(range2) * sizeof(array[0]));
			return;
		}
	}
	
	Reverse(array, range1);
	Reverse(array, range2);
	Reverse(array, range);
}

/* make sure the items within the given range are in a stable order */
void WikiVerify(const Test array[], const Range range, const Comparison compare, const char *msg) {
	long index, index2;
	for (index = range.start + 1; index < range.end; index++) {
		if (!(compare(array[index - 1], array[index]) || (!compare(array[index], array[index - 1]) && array[index].index > array[index - 1].index))) {
			for (index2 = range.start; index2 < range.end; index2++) printf("%d (%d) ", array[index2].value, array[index2].index);
			printf("failed with message: %s\n", msg);
			assert(false);
		}
	}
}

/* standard merge operation using an internal or external buffer */
void WikiMerge(Test array[], const Range buffer, const Range A, const Range B, const Comparison compare, Test cache[], const long cache_size) {
	if (Range_length(B) <= 0 || Range_length(A) <= 0) return;
	if (!compare(array[B.start], array[A.end - 1])) return;
	
	/* if A fits into the cache, use that instead of the internal buffer */
	if (Range_length(A) <= cache_size) {
		/* copy the rest of A into the buffer */
		memcpy(&cache[0], &array[A.start], Range_length(A) * sizeof(array[0]));
		
		long A_count = 0, B_count = 0, insert = 0;
		while (A_count < Range_length(A) && B_count < Range_length(B)) {
			if (!compare(array[B.start + B_count], cache[A_count])) {
				array[A.start + insert] = cache[A_count];
				A_count++;
			} else {
				array[A.start + insert] = array[B.start + B_count];
				B_count++;
			}
			insert++;
		}
		
		/* copy the remainder of A into the final array */
		memcpy(&array[A.start + insert], &cache[A_count], (Range_length(A) - A_count) * sizeof(array[0]));
	} else {
		/* swap the rest of A into the buffer */
		BlockSwap(array, buffer.start, A.start, Range_length(A));
		
		/* whenever we find a value to add to the final array, swap it with the value that's already in that spot */
		/* when this algorithm is finished, 'buffer' will contain its original contents, but in a different order */
		long A_count = 0, B_count = 0, insert = 0;
		while (true) {
			if (!compare(array[B.start + B_count], array[buffer.start + A_count])) {
				Swap(array[A.start + insert], array[buffer.start + A_count]);
				insert++;
				A_count++;
				if (A_count >= Range_length(A)) break;
			} else {
				Swap(array[A.start + insert], array[B.start + B_count]);
				insert++;
				B_count++;
				if (B_count >= Range_length(B)) break;
			}
		}
		
		/* swap the remainder of A into the final array */
		BlockSwap(array, buffer.start + A_count, A.start + insert, Range_length(A) - A_count);
	}
}

/* bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use */
void WikiSort(Test array[], const long size, const Comparison compare) {
	/* reverse any descending ranges in the array, as that will allow them to sort faster */
	double time;
	
	if (PROFILE) reverse_time = insertion_time = scale_time = rotate_time = merge_time2 = merge_time3 = min_time = pull_out_time = merge_time = insertion_time2 = redistribute_time = 0;
	
	if (PROFILE) time = Seconds();
	Range reverse = MakeRange(0, 0);
	for (long index = 1; index < size; index++) {
		if (compare(array[index], array[index - 1])) reverse.end++;
		else { Reverse(array, reverse); reverse = MakeRange(index, index); }
	}
	Reverse(array, reverse);
	if (PROFILE) reverse_time += Seconds() - time;
	
	if (size <= 32) {
		/* insertion sort the array, as there are 32 or fewer items */
		InsertionSort(array, MakeRange(0, size), compare);
		return;
	}
	
	/* use a small cache to speed up some of the operations */
	#define CACHE_SIZE 200
	const long cache_size = CACHE_SIZE;
	Test cache[CACHE_SIZE];
	
	/* calculate how to scale the index value to the range within the array */
	const long power_of_two = FloorPowerOfTwo(size);
	const long fractional_base = power_of_two/16;
	long fractional_step = size % fractional_base;
	long decimal_step = size/fractional_base;
	
	if (PROFILE) time = Seconds();
	/* first insertion sort everything the lowest level, which is 16-31 items at a time */
	long start, mid, end, decimal = 0, fractional = 0;
	for (long merge_index = 0; merge_index < power_of_two; merge_index += 16) {
		start = decimal;
		
		decimal += decimal_step;
		fractional += fractional_step;
		if (fractional >= fractional_base) { fractional -= fractional_base; decimal += 1; }
		
		end = decimal;
		
		InsertionSort(array, MakeRange(start, end), compare);
	}
	if (PROFILE) insertion_time += Seconds() - time;
	
	/* then merge sort the higher levels, which can be 32-63, 64-127, 128-255, etc. */
	for (long merge_size = 16; merge_size < power_of_two; merge_size += merge_size) {
		long block_size = sqrt(decimal_step);
		long buffer_size = decimal_step/block_size + 1;
		
		/* as an optimization, we really only need to pull out an internal buffer once for each level of merges */
		/* after that we can reuse the same buffer over and over, then redistribute it when we're finished with this level */
		Range level1 = MakeRange(0, 0), level2, levelA, levelB;
		
		decimal = fractional = 0;
		for (long merge_index = 0; merge_index < power_of_two - merge_size; merge_index += merge_size + merge_size) {
			if (PROFILE) time = Seconds();
			
			start = decimal;
			
			decimal += decimal_step;
			fractional += fractional_step;
			if (fractional >= fractional_base) { fractional -= fractional_base; decimal += 1; }
			
			mid = decimal;
			
			decimal += decimal_step;
			fractional += fractional_step;
			if (fractional >= fractional_base) { fractional -= fractional_base; decimal += 1; }
			
			end = decimal;
			
			if (PROFILE) scale_time += Seconds() - time;
			
			if (compare(array[end - 1], array[start])) {
				/* the two ranges are in reverse order, so a simple rotation should fix it */
				if (PROFILE) time = Seconds();
				Rotate(array, mid - start, MakeRange(start, end), cache, cache_size);
				if (VERIFY) WikiVerify(array, MakeRange(start, end), compare, "reversing order via Rotate()");
				if (PROFILE) rotate_time += Seconds() - time;
			} else if (compare(array[mid], array[mid - 1])) {
				/* these two ranges weren't already in order, so we'll need to merge them! */
				Range A = MakeRange(start, mid), B = MakeRange(mid, end);
				
				if (VERIFY) WikiVerify(array, A, compare, "making sure A is valid");
				if (VERIFY) WikiVerify(array, B, compare, "making sure B is valid");
				
				/* try to fill up two buffers with unique values in ascending order */
				Range bufferA, bufferB, buffer1, buffer2, blockA, blockB, firstA, lastA, lastB;
				
				if (Range_length(A) <= cache_size) {
					if (PROFILE) time = Seconds();
					WikiMerge(array, MakeRange(0, 0), A, B, compare, cache, cache_size);
					if (VERIFY) WikiVerify(array, MakeRange(A.start, B.end), compare, "using the cache to merge A and B");
					if (PROFILE) merge_time2 += Seconds() - time;
					continue;
				}
				
				if (Range_length(level1) > 0) {
					/* reuse the buffers we found in a previous iteration */
					bufferA = MakeRange(A.start, A.start);
					bufferB = MakeRange(B.end, B.end);
					buffer1 = level1;
					buffer2 = level2;
				} else {
					if (PROFILE) time = Seconds();
					/* the first item is always going to be the first unique value, so let's start searching at the next index */
					long count = 1;
					for (buffer1.start = A.start + 1; buffer1.start < A.end; buffer1.start++)
						if (compare(array[buffer1.start - 1], array[buffer1.start]) || compare(array[buffer1.start], array[buffer1.start - 1]))
							if (++count == buffer_size) break;
					buffer1.end = buffer1.start + count;
					
					/* if the size of each block fits into the cache, we only need one buffer for tagging the A blocks */
					/* this is because the other buffer is used as a swap space for merging the A blocks into the B values that follow it, */
					/* but we can just use the cache as the buffer instead. this skips some memmoves and an insertion sort */
					if (buffer_size <= cache_size) {
						buffer2 = MakeRange(A.start, A.start);
						
						if (Range_length(buffer1) == buffer_size) {
							/* we found enough values for the buffer in A */
							bufferA = MakeRange(buffer1.start, buffer1.start + buffer_size);
							bufferB = MakeRange(B.end, B.end);
							buffer1 = MakeRange(A.start, A.start + buffer_size);
						} else {
							/* we were unable to find enough unique values in A, so try B */
							bufferA = MakeRange(buffer1.start, buffer1.start);
							buffer1 = MakeRange(A.start, A.start);
							
							/* the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index */
							count = 1;
							for (buffer1.start = B.end - 2; buffer1.start >= B.start; buffer1.start--)
								if (compare(array[buffer1.start], array[buffer1.start + 1]) || compare(array[buffer1.start + 1], array[buffer1.start]))
									if (++count == buffer_size) break;
							buffer1.end = buffer1.start + count;
							
							if (Range_length(buffer1) == buffer_size) {
								bufferB = MakeRange(buffer1.start, buffer1.start + buffer_size);
								buffer1 = MakeRange(B.end - buffer_size, B.end);
							}
						}
					} else {
						/* the first item of the second buffer isn't guaranteed to be the first unique value, so we need to find the first unique item too */
						count = 0;
						for (buffer2.start = buffer1.start + 1; buffer2.start < A.end; buffer2.start++)
							if (compare(array[buffer2.start - 1], array[buffer2.start]) || compare(array[buffer2.start], array[buffer2.start - 1]))
								if (++count == buffer_size) break;
						buffer2.end = buffer2.start + count;
						
						if (Range_length(buffer2) == buffer_size) {
							/* we found enough values for both buffers in A */
							bufferA = MakeRange(buffer2.start, buffer2.start + buffer_size * 2);
							bufferB = MakeRange(B.end, B.end);
							buffer1 = MakeRange(A.start, A.start + buffer_size);
							buffer2 = MakeRange(A.start + buffer_size, A.start + buffer_size * 2);
						} else if (Range_length(buffer1) == buffer_size) {
							/* we found enough values for one buffer in A, so we'll need to find one buffer in B */
							bufferA = MakeRange(buffer1.start, buffer1.start + buffer_size);
							buffer1 = MakeRange(A.start, A.start + buffer_size);
							
							/* like before, the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index */
							count = 1;
							for (buffer2.start = B.end - 2; buffer2.start >= B.start; buffer2.start--)
								if (compare(array[buffer2.start], array[buffer2.start + 1]) || compare(array[buffer2.start + 1], array[buffer2.start]))
									if (++count == buffer_size) break;
							buffer2.end = buffer2.start + count;
							
							if (Range_length(buffer2) == buffer_size) {
								bufferB = MakeRange(buffer2.start, buffer2.start + buffer_size);
								buffer2 = MakeRange(B.end - buffer_size, B.end);
							} else buffer1.end = buffer1.start; /* failure */
						} else {
							/* we were unable to find a single buffer in A, so we'll need to find two buffers in B */
							count = 1;
							for (buffer1.start = B.end - 2; buffer1.start >= B.start; buffer1.start--)
								if (compare(array[buffer1.start], array[buffer1.start + 1]) || compare(array[buffer1.start + 1], array[buffer1.start]))
									if (++count == buffer_size) break;
							buffer1.end = buffer1.start + count;
							
							count = 0;
							for (buffer2.start = buffer1.start - 1; buffer2.start >= B.start; buffer2.start--)
								if (compare(array[buffer2.start], array[buffer2.start + 1]) || compare(array[buffer2.start + 1], array[buffer2.start]))
									if (++count == buffer_size) break;
							buffer2.end = buffer2.start + count;
							
							if (Range_length(buffer2) == buffer_size) {
								bufferA = MakeRange(A.start, A.start);
								bufferB = MakeRange(buffer2.start, buffer2.start + buffer_size * 2);
								buffer1 = MakeRange(B.end - buffer_size, B.end);
								buffer2 = MakeRange(buffer1.start - buffer_size, buffer1.start);
							} else buffer1.end = buffer1.start; /* failure */
						}
					}
					
					if (Range_length(buffer1) < buffer_size) {
						/* we failed to fill both buffers with unique values, which implies we're merging two subarrays with a lot of the same values repeated */
						/* we can use this knowledge to write a merge operation that is optimized for arrays of repeating values */
						Range oldA = A, oldB = B;
						
						while (Range_length(A) > 0 && Range_length(B) > 0) {
							/* find the first place in B where the first item in A needs to be inserted */
							long mid = BinaryFirst(array, A.start, B, compare);
							
							/* rotate A into place */
							long amount = mid - A.end;
							Rotate(array, -amount, MakeRange(A.start, mid), cache, cache_size);
							
							/* calculate the new A and B ranges */
							B.start = mid;
							A = MakeRange(BinaryLast(array, A.start + amount, A, compare), B.start);
						}
						
						if (VERIFY) WikiVerify(array, MakeRange(oldA.start, oldB.end), compare, "performing a brute-force in-place merge");
						continue;
					}
					
					/* move the unique values to the start of A if needed */
					long length = Range_length(bufferA); count = 0;
					for (long index = bufferA.start; count < length; index--) {
						if (index == A.start || compare(array[index - 1], array[index]) || compare(array[index], array[index - 1])) {
							Rotate(array, -count, MakeRange(index + 1, bufferA.start + 1), cache, cache_size);
							bufferA.start = index + count; count++;
						}
					}
					bufferA = MakeRange(A.start, bufferA.start + length);
					
					if (VERIFY) {
						WikiVerify(array, MakeRange(A.start, A.start + Range_length(bufferA)), compare, "testing values pulled out from A");
						WikiVerify(array, MakeRange(A.start + Range_length(bufferA), A.end), compare, "testing remainder of A");
					}
					
					/* move the unique values to the end of B if needed */
					length = Range_length(bufferB); count = 0;
					for (long index = bufferB.start; count < length; index++) {
						if (index == B.end - 1 || compare(array[index], array[index + 1]) || compare(array[index + 1], array[index])) {
							Rotate(array, count, MakeRange(bufferB.start, index), cache, cache_size);
							bufferB.start = index - count; count++;
						}
					}
					bufferB = MakeRange(B.end - length, B.end);
					
					if (VERIFY) {
						WikiVerify(array, MakeRange(B.end - Range_length(bufferB), B.end), compare, "testing values pulled out from B");
						WikiVerify(array, MakeRange(B.start, B.end - Range_length(bufferB)), compare, "testing remainder of B");
					}
					
					/* reuse these buffers next time! */
					level1 = buffer1;
					level2 = buffer2;
					levelA = bufferA;
					levelB = bufferB;
					
					if (PROFILE) pull_out_time += Seconds() - time;
				}
				
				if (PROFILE) time = Seconds();
				
				/* break the remainder of A into blocks. firstA is the uneven-sized first A block */
				blockA = MakeRange(bufferA.end, A.end);
				firstA = MakeRange(bufferA.end, bufferA.end + Range_length(blockA) % block_size);
				
				/* swap the second value of each A block with the value in buffer1 */
				for (long index = 0, indexA = firstA.end + 1; indexA < blockA.end; index++, indexA += block_size)
					Swap(array[buffer1.start + index], array[indexA]);
				
				/* start rolling the A blocks through the B blocks! */
				/* whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well */
				lastA = firstA;
				lastB = MakeRange(0, 0);
				blockB = MakeRange(B.start, B.start + Min(block_size, Range_length(B) - Range_length(bufferB)));
				blockA.start += Range_length(firstA);
				
				long minA = blockA.start, indexA = 0;
				
				while (true) {
					/* if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block */
					if ((Range_length(lastB) > 0 && !compare(array[lastB.end - 1], array[minA])) || Range_length(blockB) == 0) {
						/* figure out where to split the previous B block, and rotate it at the split */
						long B_split = BinaryFirst(array, minA, lastB, compare);
						long B_remaining = lastB.end - B_split;
						
						/* swap the minimum A block to the beginning of the rolling A blocks */
						BlockSwap(array, blockA.start, minA, block_size);
						
						/* we need to swap the second item of the previous A block back with its original value, which is stored in buffer1 */
						/* since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized */
						Swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
						
						double time2;
						if (PROFILE) time2 = Seconds();
						
						/* now we need to split the previous B block at B_split and insert the minimum A block in-between the two parts, using a rotation */
						Rotate(array, B_remaining, MakeRange(B_split, blockA.start + block_size), cache, cache_size);
						
						/* locally merge the previous A block with the B values that follow it, using the buffer as swap space */
						WikiMerge(array, buffer2, lastA, MakeRange(lastA.end, B_split), compare, cache, cache_size);
						
						if (PROFILE) merge_time3 += Seconds() - time2;
						
						/* now we need to update the ranges and stuff */
						lastA = MakeRange(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
						lastB = MakeRange(lastA.end, lastA.end + B_remaining);
						blockA.start += block_size;
						if (Range_length(blockA) == 0) break;
						
						/* search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!) */
						if (PROFILE) time2 = Seconds();
						minA = blockA.start + 1;
						for (long findA = minA + block_size; findA < blockA.end; findA += block_size)
							if (compare(array[findA], array[minA])) minA = findA;
						minA = minA - 1; /* decrement once to get back to the start of that A block */
						if (PROFILE) min_time += Seconds() - time2;
					} else if (Range_length(blockB) < block_size) {
						/* move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation */
						Rotate(array, -Range_length(blockB), MakeRange(blockA.start, blockB.end), cache, cache_size);
						lastB = MakeRange(blockA.start, blockA.start + Range_length(blockB));
						blockA.start += Range_length(blockB);
						blockA.end += Range_length(blockB);
						minA += Range_length(blockB);
						blockB.end = blockB.start;
					} else {
						/* roll the leftmost A block to the end by swapping it with the next B block */
						BlockSwap(array, blockA.start, blockB.start, block_size);
						lastB = MakeRange(blockA.start, blockA.start + block_size);
						if (minA == blockA.start) minA = blockA.end;
						blockA.start += block_size;
						blockA.end += block_size;
						blockB.start += block_size;
						blockB.end += block_size;
						if (blockB.end > bufferB.start) blockB.end = bufferB.start;
					}
				}
				
				/* merge the last A block with the remaining B blocks */
				WikiMerge(array, buffer2, lastA, MakeRange(lastA.end, B.end - Range_length(bufferB)), compare, cache, cache_size);
				
				if (PROFILE) merge_time += Seconds() - time;
				if (PROFILE) time = Seconds();
				
				/* when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up */
				/* insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer */
				InsertionSort(array, buffer2, compare);
				
				if (PROFILE) insertion_time2 += Seconds() - time;
				if (VERIFY) WikiVerify(array, MakeRange(A.start + Range_length(bufferA), B.end - Range_length(bufferB)), compare, "making sure the local merges worked");
			}
		}
		
		if (Range_length(level1) > 0) {
			if (PROFILE) time = Seconds();
			/* redistribute bufferA back into the array */
			long level_start = levelA.start;
			for (long index = levelA.end; Range_length(levelA) > 0; index++) {
				if (index == levelB.start || !compare(array[index], array[levelA.start])) {
					long amount = index - (levelA.end);
					Rotate(array, -amount, MakeRange(levelA.start, index), cache, cache_size);
					levelA.start += (amount + 1);
					levelA.end += amount;
					index--;
				}
			}
			if (VERIFY) WikiVerify(array, MakeRange(level_start, levelB.start), compare, "redistributed levelA back into the array");
			
			/* redistribute bufferB back into the array */
			long level_end = levelB.end;
			for (long index = levelB.start; Range_length(levelB) > 0; index--) {
				if (index == level_start || !compare(array[levelB.end - 1], array[index - 1])) {
					long amount = levelB.start - index;
					Rotate(array, amount, MakeRange(index, levelB.end), cache, cache_size);
					levelB.start -= amount;
					levelB.end -= (amount + 1);
					index++;
				}
			}
			if (VERIFY) WikiVerify(array, MakeRange(level_start, level_end), compare, "redistributed levelB back into the array");
			if (PROFILE) redistribute_time += Seconds() - time;
		}
		
		decimal_step += decimal_step;
		fractional_step += fractional_step;
		if (fractional_step >= fractional_base) {
			fractional_step -= fractional_base;
			decimal_step += 1;
		}
	}
	
	#undef CACHE_SIZE
}


/* standard merge sort, so we have a baseline for how well the in-place merge works */
void MergeSortR(Test array[], const Range range, const Comparison compare, Test buffer[]) {
	long mid, A_count = 0, B_count = 0, insert = 0;
	Range A, B;
	
	if (Range_length(range) < 32) {
		InsertionSort(array, range, compare);
		return;
	}
	
	mid = range.start + (range.end - range.start)/2;
	A = MakeRange(range.start, mid);
	B = MakeRange(mid, range.end);
	
	MergeSortR(array, A, compare, buffer);
	MergeSortR(array, B, compare, buffer);
	
	/* standard merge operation here (only A is copied to the buffer, and only the parts that weren't already where they should be) */
	A = MakeRange(BinaryLast(array, B.start, A, compare), A.end);
	memcpy(&buffer[0], &array[A.start], Range_length(A) * sizeof(array[0]));
	while (A_count < Range_length(A) && B_count < Range_length(B)) {
		if (!compare(array[A.end + B_count], buffer[A_count])) {
			array[A.start + insert++] = buffer[A_count++];
		} else {
			array[A.start + insert++] = array[A.end + B_count++];
		}
	}
	memcpy(&array[A.start + insert], &buffer[A_count], (Range_length(A) - A_count) * sizeof(array[0]));
}

void MergeSort(Test array[], const long array_count, const Comparison compare) {
	Var(buffer, Allocate(Test, array_count));
	MergeSortR(array, MakeRange(0, array_count), compare, buffer);
	free(buffer);
}


int main() {
	long total, index;
	double total_time, total_time1, total_time2;
	const long max_size = 1500000;
	Var(array1, Allocate(Test, max_size));
	Var(array2, Allocate(Test, max_size));
	Comparison compare = TestCompare;
	
	/* initialize the random-number generator */
	srand(/*time(NULL)*/ 10141985);
	
	total_time = Seconds();
	total_time1 = total_time2 = 0;
	
	for (total = 0; total < max_size; total += 2048 * 16) {
		double time1, time2;
		
		for (index = 0; index < total; index++) {
			Test item; item.index = index;
			
			/* here are some possible tests to perform on this sorter:
			 if (index == 0) item.value = 10;
			 else if (index < total/2) item.value = 11;
			 else if (index == total - 1) item.value = 10;
			 else item.value = 9;
			 
			 item.value = rand();
			 item.value = total - index + rand() * 1.0/RAND_MAX * 5 - 2.5;
			 item.value = index + rand() * 1.0/RAND_MAX * 5 - 2.5;
			 item.value = index;
			 item.value = total - index;
			 item.value = 1000;
			 item.value = (rand() * 1.0/RAND_MAX <= 0.9) ? index : (index - 2);
			 item.value = 1000 + rand() * 1.0/RAND_MAX * 4;
			 */
			
			item.value = rand();
			
			array1[index] = array2[index] = item;
		}
		
		time1 = Seconds();
		WikiSort(array1, total, compare);
		time1 = Seconds() - time1;
		total_time1 += time1;
		
		time2 = Seconds();
		MergeSort(array2, total, compare);
		time2 = Seconds() - time2;
		total_time2 += time2;
		
		printf("[%ld] wiki: %f, merge: %f (%f%%)\n", total, time1, time2, time2/time1 * 100.0);
		if (PROFILE) {
			printf("reverse: %f (%f%%)\n", reverse_time, reverse_time/time1 * 100);
			printf("insert: %f (%f%%)\n", insertion_time, insertion_time/time1 * 100);
			printf("scale: %f (%f%%)\n", scale_time, scale_time/time1 * 100);
			printf("rotate: %f (%f%%)\n", rotate_time, rotate_time/time1 * 100);
			printf("merge2: %f (%f%%)\n", merge_time2, merge_time2/time1 * 100);
			printf("merge3: %f (%f%%)\n", merge_time3, merge_time3/time1 * 100);
			printf("min: %f (%f%%)\n", min_time, min_time/time1 * 100);
			printf("pull out: %f (%f%%)\n", pull_out_time, pull_out_time/time1 * 100);
			printf("merge: %f (%f%%)\n", merge_time, merge_time/time1 * 100);
			printf("insert2: %f (%f%%)\n", insertion_time2, insertion_time2/time1 * 100);
			printf("redistribute: %f (%f%%)\n", redistribute_time, redistribute_time/time1 * 100);
		}
		
		/* make sure the arrays are sorted correctly, and that the results were stable */
		printf("verifying... ");
		fflush(stdout);
		
		WikiVerify(array1, MakeRange(0, total), compare, "testing the final array");
		if (total > 0) assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (index = 1; index < total; index++) assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		printf("correct!\n");
	}
	
	total_time = Seconds() - total_time;
	printf("tests completed in %f seconds\n", total_time);
	printf("wiki: %f, merge: %f (%f%%)\n", total_time1, total_time2, total_time2/total_time1 * 100.0);
	
	free(array1); free(array2);
	return 0;
}
