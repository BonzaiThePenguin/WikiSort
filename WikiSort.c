/***********************************************************
 WikiSort: by Arne Kutzner, Pok-Son Kim, and Mike McFadden
 https://github.com/BonzaiThePenguin/WikiSort
 
 to compile:
 clang -o WikiSort.x WikiSort.c -O3
 (or replace 'clang' with 'gcc')
***********************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <limits.h>

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
	long i;
	for (i = range.start + 1; i < range.end; i++) {
		const Test temp = array[i]; long j;
		for (j = i; j > range.start && compare(temp, array[j - 1]); j--)
			array[j] = array[j - 1];
		array[j] = temp;
	}
}

/* reverse a range within the array */
void Reverse(Test array[], const Range range) {
	long index;
	for (index = Range_length(range)/2 - 1; index >= 0; index--)
		Swap(array[range.start + index], array[range.end - index - 1]);
}

/* swap a series of values in the array */
void BlockSwap(Test array[], const long start1, const long start2, const long block_size) {
	long index;
	for (index = 0; index < block_size; index++)
		Swap(array[start1 + index], array[start2 + index]);
}

/* rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1) */
void Rotate(Test array[], const long amount, const Range range, Test cache[], const long cache_size) {
	long split; Range range1, range2;
	
	if (Range_length(range) == 0) return;
	
	if (amount >= 0)
		split = range.start + amount;
	else
		split = range.end + amount;
	
	range1 = MakeRange(range.start, split);
	range2 = MakeRange(split, range.end);
	
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

/* standard merge operation using an internal or external buffer */
void WikiMerge(Test array[], const Range buffer, const Range A, const Range B, const Comparison compare, Test cache[], const long cache_size) {
	/* if A fits into the cache, use that instead of the internal buffer */
	if (Range_length(A) <= cache_size) {
		Test *A_index = &cache[0];
		Test *B_index = &array[B.start];
		Test *insert_index = &array[A.start];
		Test *A_last = &cache[Range_length(A)];
		Test *B_last = &array[B.end];
		
		if (Range_length(B) > 0 && Range_length(A) > 0) {
			while (true) {
				if (!compare(*B_index, *A_index)) {
					*insert_index = *A_index;
					A_index++;
					insert_index++;
					if (A_index == A_last) break;
				} else {
					*insert_index = *B_index;
					B_index++;
					insert_index++;
					if (B_index == B_last) break;
				}
			}
		}
		
		/* copy the remainder of A into the final array */
		memcpy(insert_index, A_index, (A_last - A_index) * sizeof(array[0]));
	} else {
		/* whenever we find a value to add to the final array, swap it with the value that's already in that spot */
		/* when this algorithm is finished, 'buffer' will contain its original contents, but in a different order */
		long A_count = 0, B_count = 0, insert = 0;
		
		if (Range_length(B) > 0 && Range_length(A) > 0) {
			while (true) {
				if (!compare(array[B.start + B_count], array[buffer.start + A_count])) {
					Swap(array[A.start + insert], array[buffer.start + A_count]);
					A_count++;
					insert++;
					if (A_count >= Range_length(A)) break;
				} else {
					Swap(array[A.start + insert], array[B.start + B_count]);
					B_count++;
					insert++;
					if (B_count >= Range_length(B)) break;
				}
			}
		}
		
		/* swap the remainder of A into the final array */
		BlockSwap(array, buffer.start + A_count, A.start + insert, Range_length(A) - A_count);
	}
}

/* bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use */
void WikiSort(Test array[], const long size, const Comparison compare) {
	/* use a small cache to speed up some of the operations */
	/* since the cache size is fixed, it's still O(1) memory! */
	/* just keep in mind that making it too small ruins the point (nothing will fit into it), */
	/* and making it too large also ruins the point (so much for "low memory"!) */
	/* removing the cache entirely still gives 70% of the performance of a standard merge */
	#define CACHE_SIZE 512
	const long cache_size = CACHE_SIZE;
	Test cache[CACHE_SIZE];
	
	Range reverse;
	long index, merge_size, start, mid, end, fractional, decimal;
	long power_of_two, fractional_base, fractional_step, decimal_step;
	
	/* reverse any descending ranges in the array, as that will allow them to sort faster */
	reverse = MakeRange(0, 1);
	for (index = 1; index < size; index++) {
		if (compare(array[index], array[index - 1]))
			reverse.end++;
		else {
			Reverse(array, reverse);
			reverse = MakeRange(index, index + 1);
		}
	}
	Reverse(array, reverse);
	
	/* if there are 32 or fewer items, just insertion sort the entire array */
	if (size <= 32) {
		InsertionSort(array, MakeRange(0, size), compare);
		return;
	}
	
	/* calculate how to scale the index value to the range within the array */
	/* (this is essentially fixed-point math, where we manually check for and handle overflow) */
	power_of_two = FloorPowerOfTwo(size);
	fractional_base = power_of_two/16;
	fractional_step = size % fractional_base;
	decimal_step = size/fractional_base;
	
	/* first insertion sort everything the lowest level, which is 16-31 items at a time */
	decimal = 0; fractional = 0;
	while (decimal < size) {
		start = decimal;
		
		decimal += decimal_step;
		fractional += fractional_step;
		if (fractional >= fractional_base) { fractional -= fractional_base; decimal += 1; }
		
		end = decimal;
		
		InsertionSort(array, MakeRange(start, end), compare);
	}
	
	/* then merge sort the higher levels, which can be 32-63, 64-127, 128-255, etc. */
	for (merge_size = 16; merge_size < power_of_two; merge_size += merge_size) {
		long block_size = sqrt(decimal_step);
		long buffer_size = decimal_step/block_size + 1;
		
		/* as an optimization, we really only need to pull out an internal buffer once for each level of merges */
		/* after that we can reuse the same buffer over and over, then redistribute it when we're finished with this level */
		Range level1 = MakeRange(0, 0), level2, levelA, levelB;
		
		decimal = fractional = 0;
		while (decimal < size) {
			start = decimal;
			
			decimal += decimal_step;
			fractional += fractional_step;
			if (fractional >= fractional_base) { fractional -= fractional_base; decimal += 1; }
			
			mid = decimal;
			
			decimal += decimal_step;
			fractional += fractional_step;
			if (fractional >= fractional_base) { fractional -= fractional_base; decimal += 1; }
			
			end = decimal;
			
			if (compare(array[end - 1], array[start])) {
				/* the two ranges are in reverse order, so a simple rotation should fix it */
				Rotate(array, mid - start, MakeRange(start, end), cache, cache_size);
			} else if (compare(array[mid], array[mid - 1])) {
				Range bufferA, bufferB, buffer1, buffer2, blockA, blockB, firstA, lastA, lastB;
				long indexA, minA, findA;
				Test min_value;
				
				/* these two ranges weren't already in order, so we'll need to merge them! */
				Range A = MakeRange(start, mid), B = MakeRange(mid, end);
				
				/* try to fill up two buffers with unique values in ascending order */
				if (Range_length(A) <= cache_size) {
					memcpy(&cache[0], &array[A.start], Range_length(A) * sizeof(array[0]));
					WikiMerge(array, MakeRange(0, 0), A, B, compare, cache, cache_size);
					continue;
				}
				
				if (Range_length(level1) > 0) {
					/* reuse the buffers we found in a previous iteration */
					bufferA = MakeRange(A.start, A.start);
					bufferB = MakeRange(B.end, B.end);
					buffer1 = level1;
					buffer2 = level2;
					
				} else {
					long count, length;
					
					/* the first item is always going to be the first unique value, so let's start searching at the next index */
					count = 1;
					for (buffer1.start = A.start + 1; buffer1.start < A.end; buffer1.start++)
						if (compare(array[buffer1.start - 1], array[buffer1.start]) || compare(array[buffer1.start], array[buffer1.start - 1]))
							if (++count == buffer_size)
								break;
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
									if (++count == buffer_size)
										break;
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
								if (++count == buffer_size)
									break;
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
									if (++count == buffer_size)
										break;
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
									if (++count == buffer_size)
										break;
							buffer1.end = buffer1.start + count;
							
							count = 0;
							for (buffer2.start = buffer1.start - 1; buffer2.start >= B.start; buffer2.start--)
								if (compare(array[buffer2.start], array[buffer2.start + 1]) || compare(array[buffer2.start + 1], array[buffer2.start]))
									if (++count == buffer_size)
										break;
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
						
						continue;
					}
					
					/* move the unique values to the start of A if needed */
					length = Range_length(bufferA);
					count = 0;
					for (index = bufferA.start; count < length; index--) {
						if (index == A.start || compare(array[index - 1], array[index]) || compare(array[index], array[index - 1])) {
							Rotate(array, -count, MakeRange(index + 1, bufferA.start + 1), cache, cache_size);
							bufferA.start = index + count; count++;
						}
					}
					bufferA = MakeRange(A.start, A.start + length);
					
					/* move the unique values to the end of B if needed */
					length = Range_length(bufferB);
					count = 0;
					for (index = bufferB.start; count < length; index++) {
						if (index == B.end - 1 || compare(array[index], array[index + 1]) || compare(array[index + 1], array[index])) {
							Rotate(array, count, MakeRange(bufferB.start, index), cache, cache_size);
							bufferB.start = index - count; count++;
						}
					}
					bufferB = MakeRange(B.end - length, B.end);
					
					/* reuse these buffers next time! */
					level1 = buffer1;
					level2 = buffer2;
					levelA = bufferA;
					levelB = bufferB;
				}
				
				/* break the remainder of A into blocks. firstA is the uneven-sized first A block */
				blockA = MakeRange(bufferA.end, A.end);
				firstA = MakeRange(bufferA.end, bufferA.end + Range_length(blockA) % block_size);
				
				/* swap the second value of each A block with the value in buffer1 */
				index = 0;
				for (indexA = firstA.end + 1; indexA < blockA.end; index++, indexA += block_size)
					Swap(array[buffer1.start + index], array[indexA]);
				
				/* start rolling the A blocks through the B blocks! */
				/* whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well */
				lastA = firstA;
				lastB = MakeRange(0, 0);
				blockB = MakeRange(B.start, B.start + Min(block_size, Range_length(B) - Range_length(bufferB)));
				blockA.start += Range_length(firstA);
				
				minA = blockA.start;
				min_value = array[minA];
				indexA = 0;
				
				if (Range_length(lastA) <= cache_size)
					memcpy(&cache[0], &array[lastA.start], Range_length(lastA) * sizeof(array[0]));
				else
					BlockSwap(array, lastA.start, buffer2.start, Range_length(lastA));
				
				while (true) {
					/* if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block */
					if ((Range_length(lastB) > 0 && !compare(array[lastB.end - 1], min_value)) || Range_length(blockB) == 0) {
						/* figure out where to split the previous B block, and rotate it at the split */
						long B_split = BinaryFirst(array, minA, lastB, compare);
						long B_remaining = lastB.end - B_split;
						
						/* swap the minimum A block to the beginning of the rolling A blocks */
						BlockSwap(array, blockA.start, minA, block_size);
						
						/* we need to swap the second item of the previous A block back with its original value, which is stored in buffer1 */
						/* since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized */
						Swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
						
						/* locally merge the previous A block with the B values that follow it, using the buffer as swap space */
						WikiMerge(array, buffer2, lastA, MakeRange(lastA.end, B_split), compare, cache, cache_size);
						
						/* copy the previous A block into the cache or buffer2, since that's where we need it to be when we go to merge it anyway */
						if (block_size <= cache_size)
							memcpy(&cache[0], &array[blockA.start], block_size * sizeof(array[0]));
						else
							BlockSwap(array, blockA.start, buffer2.start, block_size);
						
						/* this is equivalent to rotating, but faster */
						/* the area normally taken up by the A block is either the contents of buffer2, or data we don't need anymore since we memcopied it */
						/* either way, we don't need to retain the order of those items, so instead of rotating we can just block swap B to where it belongs */
						BlockSwap(array, B_split, blockA.start + block_size - B_remaining, B_remaining);
						
						/* now we need to update the ranges and stuff */
						lastA = MakeRange(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
						lastB = MakeRange(lastA.end, lastA.end + B_remaining);
						blockA.start += block_size;
						if (Range_length(blockA) == 0)
							break;
						
						/* search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!) */
						minA = blockA.start + 1;
						for (findA = minA + block_size; findA < blockA.end; findA += block_size)
							if (compare(array[findA], array[minA])) minA = findA;
						minA = minA - 1; /* decrement once to get back to the start of that A block */
						min_value = array[minA];
						
					} else if (Range_length(blockB) < block_size) {
						/* move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation */
						/* (using the cache is disabled since we have the contents of the previous A block in it!) */
						Rotate(array, -Range_length(blockB), MakeRange(blockA.start, blockB.end), cache, 0);
						lastB = MakeRange(blockA.start, blockA.start + Range_length(blockB));
						blockA.start += Range_length(blockB);
						blockA.end += Range_length(blockB);
						minA += Range_length(blockB);
						blockB.end = blockB.start;
						
					} else {
						/* roll the leftmost A block to the end by swapping it with the next B block */
						BlockSwap(array, blockA.start, blockB.start, block_size);
						lastB = MakeRange(blockA.start, blockA.start + block_size);
						if (minA == blockA.start)
							minA = blockA.end;
						
						blockA.start += block_size;
						blockA.end += block_size;
						blockB.start += block_size;
						blockB.end += block_size;
						if (blockB.end > bufferB.start)
							blockB.end = bufferB.start;
					}
				}
				
				/* merge the last A block with the remaining B blocks */
				WikiMerge(array, buffer2, lastA, MakeRange(lastA.end, B.end - Range_length(bufferB)), compare, cache, cache_size);
			}
		}
		
		if (Range_length(level1) > 0) {
			long level_start;
			
			/* when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up */
			/* insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer */
			InsertionSort(array, level2, compare);
			
			/* redistribute bufferA back into the array */
			level_start = levelA.start;
			for (index = levelA.end; Range_length(levelA) > 0; index++) {
				if (index == levelB.start || !compare(array[index], array[levelA.start])) {
					long amount = index - levelA.end;
					Rotate(array, -amount, MakeRange(levelA.start, index), cache, cache_size);
					levelA.start += (amount + 1);
					levelA.end += amount;
					index--;
				}
			}
			
			/* redistribute bufferB back into the array */
			for (index = levelB.start; Range_length(levelB) > 0; index--) {
				if (index == level_start || !compare(array[levelB.end - 1], array[index - 1])) {
					long amount = levelB.start - index;
					Rotate(array, amount, MakeRange(index, levelB.end), cache, cache_size);
					levelB.start -= amount;
					levelB.end -= (amount + 1);
					index++;
				}
			}
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
			array[A.start + insert] = buffer[A_count];
			A_count++;
		} else {
			array[A.start + insert] = array[A.end + B_count];
			B_count++;
		}
		insert++;
	}
	
	memcpy(&array[A.start + insert], &buffer[A_count], (Range_length(A) - A_count) * sizeof(array[0]));
}

void MergeSort(Test array[], const long array_count, const Comparison compare) {
	Var(buffer, Allocate(Test, array_count));
	MergeSortR(array, MakeRange(0, array_count), compare, buffer);
	free(buffer);
}


long TestingPathological(long index, long total) {
	if (index == 0) return 10;
	else if (index < total/2) return 11;
	else if (index == total - 1) return 10;
	return 9;
}

long TestingRandom(long index, long total) {
	return rand();
}

long TestingMostlyDescending(long index, long total) {
	return total - index + rand() * 1.0/RAND_MAX * 5 - 2.5;
}

long TestingMostlyAscending(long index, long total) {
	return index + rand() * 1.0/RAND_MAX * 5 - 2.5;
}

long TestingAscending(long index, long total) {
	return index;
}

long TestingDescending(long index, long total) {
	return total - index;
}

long TestingEqual(long index, long total) {
	return 1000;
}

long TestingJittered(long index, long total) {
	return (rand() * 1.0/RAND_MAX <= 0.9) ? index : (index - 2);
}

long TestingMostlyEqual(long index, long total) {
	return 1000 + rand() * 1.0/RAND_MAX * 4;
}


/* make sure the items within the given range are in a stable order */
void WikiVerify(const Test array[], const Range range, const Comparison compare, const char *msg) {
	long index, index2;
	for (index = range.start + 1; index < range.end; index++) {
		/* if it's in ascending order then we're good */
		/* if both values are equal, we need to make sure the index values are ascending */
		if (!(compare(array[index - 1], array[index]) ||
			  (!compare(array[index], array[index - 1]) && array[index].index > array[index - 1].index))) {
			
			for (index2 = range.start; index2 < range.end; index2++)
				printf("%d (%d) ", array[index2].value, array[index2].index);
			
			printf("failed with message: %s\n", msg);
			assert(false);
		}
	}
}

int main() {
	long total, index, test_case;
	double total_time, total_time1, total_time2;
	const long max_size = 1500000;
	Var(array1, Allocate(Test, max_size));
	Var(array2, Allocate(Test, max_size));
	Comparison compare = TestCompare;
	
	__typeof__(&TestingPathological) test_cases[] = {
		TestingPathological,
		TestingRandom,
		TestingMostlyDescending,
		TestingMostlyAscending,
		TestingAscending,
		TestingDescending,
		TestingEqual,
		TestingJittered,
		TestingMostlyEqual
	};
	
	/* initialize the random-number generator */
	srand(/*time(NULL)*/ 10141985);
	
	printf("running test cases... ");
	fflush(stdout);
	
	total = max_size;
	for (test_case = 0; test_case < sizeof(test_cases)/sizeof(test_cases[0]); test_case++) {
		
		for (index = 0; index < total; index++) {
			Test item;
			
			item.value = test_cases[test_case](index, total);
			item.index = index;
			
			array1[index] = array2[index] = item;
		}
		
		WikiSort(array1, total, compare);
		
		MergeSort(array2, total, compare);
		
		WikiVerify(array1, MakeRange(0, total), compare, "test case failed");
		if (total > 0)
			assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (index = 1; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
	}
	printf("passed!\n");
	
	total_time = Seconds();
	total_time1 = total_time2 = 0;
	
	for (total = 0; total < max_size; total += 2048 * 16) {
		double time1, time2;
		
		for (index = 0; index < total; index++) {
			Test item;
			
			item.index = index;
			item.value = TestingRandom(index, total);
			
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
		
		/* make sure the arrays are sorted correctly, and that the results were stable */
		printf("verifying... ");
		fflush(stdout);
		
		WikiVerify(array1, MakeRange(0, total), compare, "testing the final array");
		if (total > 0)
			assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (index = 1; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		
		printf("correct!\n");
	}
	
	total_time = Seconds() - total_time;
	printf("tests completed in %f seconds\n", total_time);
	printf("wiki: %f, merge: %f (%f%%)\n", total_time1, total_time2, total_time2/total_time1 * 100.0);
	
	free(array1); free(array2);
	return 0;
}
