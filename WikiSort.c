/***********************************************************
 WikiSort (public domain license)
 https://github.com/BonzaiThePenguin/WikiSort
 
 to run:
 clang -o WikiSort.x WikiSort.c -O3
 (or replace 'clang' with 'gcc')
 ./WikiSort.x
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
long BinaryFirst(const Test array[], const Test value, const Range range, const Comparison compare) {
	long start = range.start, end = range.end - 1;
	while (start < end) {
		long mid = start + (end - start)/2;
		if (compare(array[mid], value))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.end - 1 && compare(array[start], value)) start++;
	return start;
}

/* find the index of the last value within the range that is equal to array[index], plus 1 */
long BinaryLast(const Test array[], const Test value, const Range range, const Comparison compare) {
	long start = range.start, end = range.end - 1;
	while (start < end) {
		long mid = start + (end - start)/2;
		if (!compare(value, array[mid]))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.end - 1 && !compare(value, array[start])) start++;
	return start;
}

/* n^2 sorting algorithm used to sort tiny chunks of the full array */
void InsertionSort(Test array[], const Range range, const Comparison compare) {
	long i;
	for (i = range.start + 1; i < range.end; i++) {
		const Test temp = array[i];
		long j;
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

/* merge operation using an external buffer, */
void MergeExternal(Test array[], const Range A, const Range B, const Comparison compare, Test cache[], const long cache_size) {
	/* A fits into the cache, so use that instead of the internal buffer */
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
}

/* merge operation using an internal buffer */
void MergeInternal(Test array[], const Range A, const Range B, const Comparison compare, const Range buffer) {
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

/* merge operation without a buffer */
void MergeInPlace(Test array[], Range A, Range B, const Comparison compare) {
	/* this just repeatedly binary searches into B and rotates A into position, */
	/* although the paper suggests using the "rotation-based variant of the Hwang and Lin algorithm" */
	
	while (Range_length(A) > 0 && Range_length(B) > 0) {
		/* find the first place in B where the first item in A needs to be inserted */
		long mid = BinaryFirst(array, array[A.start], B, compare);
		
		/* rotate A into place */
		long amount = mid - A.end;
		Rotate(array, -amount, MakeRange(A.start, mid), array, 0);
		
		/* calculate the new A and B ranges */
		B.start = mid;
		A = MakeRange(BinaryLast(array, array[A.start + amount], A, compare), B.start);
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
	
	/* also, if you change this to dynamically allocate a full-size buffer, */
	/* the algorithm seamlessly turns into a full-speed standard merge sort! */
	/* const long cache_size = (size + 1)/2; */
	/* etc. */
	
	long merge_size, start, mid, end, fractional, decimal;
	long power_of_two, fractional_base, fractional_step, decimal_step;
	
	/* if there are 32 or fewer items, just insertion sort the entire array */
	if (size <= 32) {
		InsertionSort(array, MakeRange(0, size), compare);
		return;
	}
	
	/* calculate how to scale the index value to the range within the array */
	/* this is essentially 64.64 fixed-point math, where we manually check for and handle overflow, */
	/* and where the fractional part is in base "fractional_base", rather than base 10 */
	power_of_two = FloorPowerOfTwo(size);
	fractional_base = power_of_two/16;
	fractional_step = size % fractional_base;
	decimal_step = size/fractional_base;
	
	/* first insertion sort everything the lowest level, which is 16-31 items at a time */
	decimal = 0;
	fractional = 0;
	while (decimal < size) {
		start = decimal;
		
		decimal += decimal_step;
		fractional += fractional_step;
		if (fractional >= fractional_base) {
			fractional -= fractional_base;
			decimal += 1;
		}
		
		end = decimal;
		
		InsertionSort(array, MakeRange(start, end), compare);
	}
	
	/* then merge sort the higher levels, which can be 16-31, 32-63, 64-127, 128-255, etc. */
	for (merge_size = 16; merge_size < power_of_two; merge_size += merge_size) {
		/* if every A and B block will fit into the cache, use a special branch specifically for merging with the cache */
		/* (we use < rather than <= since the block size might be one more than decimal_step) */
		if (decimal_step < cache_size) {
			decimal = fractional = 0;
			while (decimal < size) {
				start = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				mid = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				end = decimal;
				
				if (compare(array[mid], array[mid - 1])) {
					/* these two ranges weren't already in order, so we'll need to merge them! */
					memcpy(&cache[0], &array[start], (mid - start) * sizeof(array[0]));
					MergeExternal(array, MakeRange(start, mid), MakeRange(mid, end), compare, cache, cache_size);
				}
			}
		} else {
			/* this is where the in-place merge logic starts! */
			/* 1. pull out two internal buffers each containing √A unique values */
			/*     1a. adjust block_size and buffer_size if we couldn't find enough unique values */
			/* 2. loop over the A and B areas within this level of the merge sort */
			/*     3. break A and B into blocks of size 'block_size' */
			/*     4. "tag" each of the A blocks with values from the first internal buffer */
			/*     5. roll the A blocks through the B blocks and drop/rotate them where they belong */
			/*     6. merge each A block with any B values that follow, using the cache or second the internal buffer */
			/* 7. sort the second internal buffer if it exists */
			/* 8. redistribute the two internal buffers back into the array */
			
			long block_size = sqrt(decimal_step);
			long buffer_size = decimal_step/block_size + 1;
			
			/* as an optimization, we really only need to pull out the internal buffers once for each level of merges */
			/* after that we can reuse the same buffers over and over, then redistribute it when we're finished with this level */
			Range buffer1, buffer2, A, B;
			long index, last, count, pull_index = 0, find = buffer_size + buffer_size;
			struct { long from, to, count; Range range; } pull[2] = { { 0 }, { 0 } };
			
			buffer1 = MakeRange(0, 0);
			buffer2 = MakeRange(0, 0);
			
			/* we need to find either a single contiguous space containing 2√A unique values (which will be split up into two buffers of size √A each), */
			/* or we need to find one buffer of < 2√A unique values, and a second buffer of √A unique values, */
			/* OR if we couldn't find that many unique values, we need the largest possible buffer we can get */
			
			/* in the case where it couldn't find a single buffer of at least √A unique values, */
			/* all of the Merge steps must be replaced by a different merge algorithm (MergeInPlace) */
			decimal = fractional = 0;
			while (decimal < size) {
				start = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				mid = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				end = decimal;
				
				/* check A (from start to mid) for the number of unique values we need to fill an internal buffer */
				/* these values will be pulled out to the start of A */
				last = start;
				count = 1;
				for (index = start + 1; index < mid; index++) {
					if (compare(array[index - 1], array[index])) {
						last = index;
						if (++count >= find) break;
					}
				}
				index = last;
				
				if (count >= buffer_size) {
					/* keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer */
					pull[pull_index].range = MakeRange(start, end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = start;
					pull_index = 1;
					
					if (count == buffer_size + buffer_size) {
						/* we were able to find a single contiguous section containing 2√A unique values, */
						/* so this section can be used to contain both of the internal buffers we'll need */
						buffer1 = MakeRange(start, start + buffer_size);
						buffer2 = MakeRange(start + buffer_size, start + count);
						break;
					} else if (find == buffer_size + buffer_size) {
						buffer1 = MakeRange(start, start + count);
						
						/* we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values, */
						/* so we still need to find a second separate buffer of at least √A unique values */
						find = buffer_size;
					} else {
						/* we found a second buffer in an 'A' area containing √A unique values, so we're done! */
						buffer2 = MakeRange(start, start + count);
						break;
					}
				} else if (pull_index == 0 && count > Range_length(buffer1)) {
					/* keep track of the largest buffer we were able to find */
					buffer1 = MakeRange(start, start + count);
					
					pull[pull_index].range = MakeRange(start, end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = start;
				}
				
				/* check B (from mid to end) for the number of unique values we need to fill an internal buffer */
				/* these values will be pulled out to the end of B */
				last = end - 1;
				count = 1;
				for (index = end - 2; index >= mid; index--) {
					if (compare(array[index], array[index + 1])) {
						last = index;
						if (++count >= find) break;
					}
				}
				index = last;
				
				if (count >= buffer_size) {
					/* keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer */
					pull[pull_index].range = MakeRange(start, end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = end;
					pull_index = 1;
					
					if (count == buffer_size + buffer_size) {
						/* we were able to find a single contiguous section containing 2√A unique values, */
						/* so this section can be used to contain both of the internal buffers we'll need */
						buffer1 = MakeRange(end - count, end - buffer_size);
						buffer2 = MakeRange(end - buffer_size, end);
						break;
					} else if (find == buffer_size + buffer_size) {
						buffer1 = MakeRange(end - count, end);
						
						/* we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values, */
						/* so we still need to find a second separate buffer of at least √A unique values */
						find = buffer_size;
					} else {
						/* we found a second buffer in an 'B' area containing √A unique values, so we're done! */
						buffer2 = MakeRange(end - count, end);
						
						/* buffer2 will be pulled out from a 'B' area, so if the first buffer was pulled out from the corresponding 'A' area, */
						/* we need to adjust the end point for that A area so it knows to stop redistributing its values before reaching buffer2 */
						if (pull[0].range.start == start) pull[0].range.end -= pull[1].count;
						
						break;
					}
				} else if (pull_index == 0 && count > Range_length(buffer1)) {
					/* keep track of the largest buffer we were able to find */
					buffer1 = MakeRange(end - count, end);
					
					pull[pull_index].range = MakeRange(start, end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = end;
				}
			}
			
			/* pull out the two ranges so we can use them as internal buffers */
			for (pull_index = 0; pull_index < 2; pull_index++) {
				long length = pull[pull_index].count;
				count = 0;
				
				if (pull[pull_index].to < pull[pull_index].from) {
					/* we're pulling the values out to the left, which means the start of an A area */
					for (index = pull[pull_index].from; count < length; index--) {
						if (index == pull[pull_index].to || compare(array[index - 1], array[index])) {
							Rotate(array, -count, MakeRange(index + 1, pull[pull_index].from + 1), cache, cache_size);
							pull[pull_index].from = index + count;
							count++;
						}
					}
				} else if (pull[pull_index].to > pull[pull_index].from) {
					/* we're pulling values out to the right, which means the end of a B area */
					for (index = pull[pull_index].from; count < length; index++) {
						if (index == pull[pull_index].to - 1 || compare(array[index], array[index + 1])) {
							Rotate(array, count, MakeRange(pull[pull_index].from, index), cache, cache_size);
							pull[pull_index].from = index - count;
							count++;
						}
					}
				}
			}
			
			/* adjust block_size and buffer_size based on the values we were able to pull out */
			buffer_size = Range_length(buffer1);
			block_size = decimal_step/buffer_size + 1;
			
			/* the first buffer NEEDS to be large enough to tag each of the evenly sized A blocks, */
			/* so this was originally here to test the math for adjusting block_size above */
			/* assert((decimal_step + 1)/block_size <= buffer_size); */
			
			/* now that the two internal buffers have been created, it's time to merge each A+B combination at this level of the merge sort! */
			decimal = fractional = 0;
			while (decimal < size) {
				start = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				mid = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				end = decimal;
				
				/* calculate the ranges for A and B, and make sure to remove any portions that are being used by the internal buffers */
				A = MakeRange(start, mid);
				B = MakeRange(mid, end);
				
				for (pull_index = 0; pull_index < 2; pull_index++) {
					if (start == pull[pull_index].range.start) {
						if (pull[pull_index].from > pull[pull_index].to)
							A.start += pull[pull_index].count;
						else if (pull[pull_index].from < pull[pull_index].to)
							B.end -= pull[pull_index].count;
					}
				}
				
				if (compare(array[B.end - 1], array[A.start])) {
					/* the two ranges are in reverse order, so a simple rotation should fix it */
					Rotate(array, A.end - A.start, MakeRange(A.start, B.end), cache, cache_size);
				} else if (compare(array[A.end], array[A.end - 1])) {
					/* these two ranges weren't already in order, so we'll need to merge them! */
					Range blockA, firstA, lastA, lastB, blockB;
					long minA, indexA, findA;
					__typeof__(array[0]) min_value;
					
					/* break the remainder of A into blocks. firstA is the uneven-sized first A block */
					blockA = MakeRange(A.start, A.end);
					firstA = MakeRange(A.start, A.start + Range_length(blockA) % block_size);
					
					/* swap the second value of each A block with the value in buffer1 */
					for (index = 0, indexA = firstA.end + 1; indexA < blockA.end; index++, indexA += block_size) 
						Swap(array[buffer1.start + index], array[indexA]);
					
					/* start rolling the A blocks through the B blocks! */
					/* whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well */
					lastA = firstA;
					lastB = MakeRange(0, 0);
					blockB = MakeRange(B.start, B.start + Min(block_size, Range_length(B)));
					blockA.start += Range_length(firstA);
					
					minA = blockA.start;
					indexA = 0;
					min_value = array[minA];
					
					/* if the first unevenly sized A block fits into the cache, copy it there for when we go to Merge it */
					/* otherwise, if the second buffer is available, block swap the contents into that */
					if (Range_length(lastA) <= cache_size)
						memcpy(&cache[0], &array[lastA.start], Range_length(lastA) * sizeof(array[0]));
					else if (Range_length(buffer2) > 0)
						BlockSwap(array, lastA.start, buffer2.start, Range_length(lastA));
					
					while (true) {
						/* if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block, */
						/* then drop that minimum A block behind. or if there are no B blocks left then keep dropping the remaining A blocks. */
						if ((Range_length(lastB) > 0 && !compare(array[lastB.end - 1], min_value)) || Range_length(blockB) == 0) {
							/* figure out where to split the previous B block, and rotate it at the split */
							long B_split = BinaryFirst(array, min_value, lastB, compare);
							long B_remaining = lastB.end - B_split;
							
							/* swap the minimum A block to the beginning of the rolling A blocks */
							BlockSwap(array, blockA.start, minA, block_size);
							
							/* we need to swap the second item of the previous A block back with its original value, which is stored in buffer1 */
							/* since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized */
							Swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
							
							/* locally merge the previous A block with the B values that follow it, using the buffer as swap space */
							if (Range_length(lastA) <= cache_size)
								MergeExternal(array, lastA, MakeRange(lastA.end, B_split), compare, cache, cache_size);
							else if (Range_length(buffer2) > 0)
								MergeInternal(array, lastA, MakeRange(lastA.end, B_split), compare, buffer2);
							else
								MergeInPlace(array, lastA, MakeRange(lastA.end, B_split), compare);
							
							if (Range_length(buffer2) > 0 || block_size <= cache_size) {
								/* copy the previous A block into the cache or buffer2, since that's where we need it to be when we go to merge it anyway */
								if (block_size <= cache_size)
									memcpy(&cache[0], &array[blockA.start], block_size * sizeof(array[0]));
								else
									BlockSwap(array, blockA.start, buffer2.start, block_size);
								
								/* this is equivalent to rotating, but faster */
								/* the area normally taken up by the A block is either the contents of buffer2, or data we don't need anymore since we memcopied it */
								/* either way, we don't need to retain the order of those items, so instead of rotating we can just block swap B to where it belongs */
								BlockSwap(array, B_split, blockA.start + block_size - B_remaining, B_remaining);
							} else {
								/* we are unable to use the 'buffer2' trick to speed up the rotation operation since buffer2 doesn't exist, so perform a normal rotation */
								Rotate(array, blockA.start - B_split, MakeRange(B_split, blockA.start + block_size), cache, cache_size);
							}
							
							/* now we need to update the ranges and stuff */
							lastA = MakeRange(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
							lastB = MakeRange(lastA.end, lastA.end + B_remaining);
							
							blockA.start += block_size;
							if (Range_length(blockA) == 0)
								break;
							
							/* search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!) */
							minA = blockA.start + 1;
							for (findA = minA + block_size; findA < blockA.end; findA += block_size)
								if (compare(array[findA], array[minA]))
									minA = findA;
							minA = minA - 1; /* decrement once to get back to the start of that A block */
							min_value = array[minA];
							
						} else if (Range_length(blockB) < block_size) {
							/* move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation */
							/* the cache is disabled here since it might contain the contents of the previous A block */
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
							
							if (blockB.end > B.end)
								blockB.end = B.end;
						}
					}
					
					/* merge the last A block with the remaining B values */
					if (Range_length(lastA) <= cache_size)
						MergeExternal(array, lastA, MakeRange(lastA.end, B.end), compare, cache, cache_size);
					else if (Range_length(buffer2) > 0)
						MergeInternal(array, lastA, MakeRange(lastA.end, B.end), compare, buffer2);
					else
						MergeInPlace(array, lastA, MakeRange(lastA.end, B.end), compare);
				}
			}
			
			/* when we're finished with this merge step we should have the one or two internal buffers left over, where the second buffer is all jumbled up */
			/* insertion sort the second buffer, then redistribute the buffers back into the array using the opposite process used for creating the buffer */
			
			/* while an unstable sort like quicksort could be applied here, in benchmarks it was consistently slightly slower than a simple insertion sort, */
			/* even for tens of millions of items. this may be because insertion sort is quite fast when the data is already somewhat sorted, like it is here */
			InsertionSort(array, buffer2, compare);
			
			for (pull_index = 0; pull_index < 2; pull_index++) {
				if (pull[pull_index].from > pull[pull_index].to) {
					/* the values were pulled out to the left, so redistribute them back to the right */
					Range buffer = MakeRange(pull[pull_index].range.start, pull[pull_index].range.start + pull[pull_index].count);
					for (index = buffer.end; Range_length(buffer) > 0; index++) {
						if (index == pull[pull_index].range.end || !compare(array[index], array[buffer.start])) {
							long amount = index - buffer.end;
							Rotate(array, -amount, MakeRange(buffer.start, index), cache, cache_size);
							buffer.start += (amount + 1);
							buffer.end += amount;
							index--;
						}
					}
				} else if (pull[pull_index].from < pull[pull_index].to) {
					/* the values were pulled out to the right, so redistribute them back to the left */
					Range buffer = MakeRange(pull[pull_index].range.end - pull[pull_index].count, pull[pull_index].range.end);
					for (index = buffer.start; Range_length(buffer) > 0; index--) {
						if (index == pull[pull_index].range.start || !compare(array[buffer.end - 1], array[index - 1])) {
							long amount = buffer.start - index;
							Rotate(array, amount, MakeRange(index, buffer.end), cache, cache_size);
							buffer.start -= amount;
							buffer.end -= (amount + 1);
							index++;
						}
					}
				}
			}
		}
		
		/* double the size of each A and B area that will be merged in the next level */
		decimal_step += decimal_step;
		fractional_step += fractional_step;
		if (fractional_step >= fractional_base) {
			fractional_step -= fractional_base;
			decimal_step++;
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
	A = MakeRange(BinaryLast(array, array[B.start], A, compare), A.end);
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
	Var(buffer, Allocate(Test, (array_count + 1)/2));
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

/* random distribution of few values was a problem with the last version, but it's better now */
/* although the algorithm in the Merge function still isn't the one the paper suggests using! */
long TestingRandomFew(long index, long total) {
	return rand() * 100.0/RAND_MAX;
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
		TestingRandomFew,
		TestingMostlyDescending,
		TestingMostlyAscending,
		TestingAscending,
		TestingDescending,
		TestingEqual,
		TestingJittered,
		TestingMostlyEqual
	};
	
	/* initialize the random-number generator */
	srand(time(NULL));
	/*srand(10141985);*/ /* in case you want the same random numbers */
	
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
		for (index = 0; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
	}
	printf("passed!\n");
	
	total_time = Seconds();
	total_time1 = total_time2 = 0;
	
	for (total = 0; total < max_size; total += 2048 * 16) {
		double time1, time2;
		
		for (index = 0; index < total; index++) {
			Test item;
			
			/* TestingPathological */
			/* TestingRandom */
			/* TestingRandomFew */
			/* TestingMostlyDescending */
			/* TestingMostlyAscending */
			/* TestingAscending */
			/* TestingDescending */
			/* TestingEqual */
			/* TestingJittered */
			/* TestingMostlyEqual */
			
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
		
		printf("[%ld] WikiSort: %f seconds, MergeSort: %f seconds (%f%%)\n", total, time1, time2, time2/time1 * 100.0);
		
		/* make sure the arrays are sorted correctly, and that the results were stable */
		printf("verifying... ");
		fflush(stdout);
		
		WikiVerify(array1, MakeRange(0, total), compare, "testing the final array");
		for (index = 0; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		
		printf("correct!\n");
	}
	
	total_time = Seconds() - total_time;
	printf("tests completed in %f seconds\n", total_time);
	printf("WikiSort: %f seconds, MergeSort: %f seconds (%f%%)\n", total_time1, total_time2, total_time2/total_time1 * 100.0);
	
	free(array1); free(array2);
	return 0;
}
