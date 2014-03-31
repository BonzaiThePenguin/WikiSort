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

/* record the number of comparisons */
#define PROFILE true

/* simulate comparisons that have a bit more overhead than just an inlined (int < int) */
#define SLOW_COMPARISONS true

double Seconds() { return clock() * 1.0/CLOCKS_PER_SEC; }


/* various #defines for the C code */
#ifndef true
	#define true 1
	#define false 0
	typedef uint8_t bool;
#endif

#define Var(name, value)				__typeof__(value) name = value
#define Allocate(type, count)				(type *)malloc((count) * sizeof(type))

size_t Min(const size_t a, const size_t b) {
	if (a < b) return a;
	return b;
}

size_t Max(const size_t a, const size_t b) {
	if (a > b) return a;
	return b;
}


/* structure to test stable sorting (index will contain its original index in the array, to make sure it doesn't switch places with other items) */
typedef struct {
	size_t value;
	size_t index;
} Test;

#if PROFILE
	/* global for testing how many comparisons are performed for each sorting algorithm */
	size_t comparisons;
#endif

#if SLOW_COMPARISONS
	#define NOOP_SIZE 100
	size_t noop1[NOOP_SIZE], noop2[NOOP_SIZE];
#endif

bool TestCompare(Test item1, Test item2) {
	#if SLOW_COMPARISONS
		/* test slow comparisons by adding some fake overhead */
		/* (in real-world use this might be string comparisons, etc.) */
		size_t index;
		for (index = 0; index < NOOP_SIZE; index++)
			noop1[index] = noop2[index];
	#endif
	
	#if PROFILE
		comparisons++;
	#endif
	
	return (item1.value < item2.value);
}

typedef bool (*Comparison)(Test, Test);



/* structure to represent ranges within the array */
typedef struct {
	size_t start;
	size_t end;
} Range;

size_t Range_length(Range range) { return range.end - range.start; }

Range Range_new(const size_t start, const size_t end) {
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
/* this comes from Hacker's Delight */
size_t FloorPowerOfTwo (const size_t value) {
	size_t x = value;
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
size_t BinaryFirst(const Test array[], const Test value, const Range range, const Comparison compare) {
	size_t start = range.start, end = range.end - 1;
	if (range.start >= range.end) return range.start;
	while (start < end) {
		size_t mid = start + (end - start)/2;
		if (compare(array[mid], value))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.end - 1 && compare(array[start], value)) start++;
	return start;
}

/* find the index of the last value within the range that is equal to array[index], plus 1 */
size_t BinaryLast(const Test array[], const Test value, const Range range, const Comparison compare) {
	size_t start = range.start, end = range.end - 1;
	if (range.start >= range.end) return range.end;
	while (start < end) {
		size_t mid = start + (end - start)/2;
		if (!compare(value, array[mid]))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.end - 1 && !compare(value, array[start])) start++;
	return start;
}

/* combine a linear search with a binary search to reduce the number of comparisons in situations */
/* where have some idea as to how many unique values there are and where the next value might be */
size_t FindFirstForward(const Test array[], const Test value, const Range range, const Comparison compare, const size_t unique) {
	size_t skip = Range_length(range)/unique, index = range.start + skip;
	while (compare(array[index - 1], value)) {
		if (index >= range.end - skip) {
			skip = range.end - index;
			index = range.end;
			break;
		}
		index += skip;
	}
	
	return BinaryFirst(array, value, Range_new(index - skip, index), compare);
}

size_t FindLastForward(const Test array[], const Test value, const Range range, const Comparison compare, const size_t unique) {
	size_t skip = Range_length(range)/unique, index = range.start + skip;
	while (!compare(value, array[index - 1])) {
		if (index >= range.end - skip) {
			skip = range.end - index;
			index = range.end;
			break;
		}
		index += skip;
	}
	
	return BinaryLast(array, value, Range_new(index - skip, index), compare);
}

size_t FindFirstBackward(const Test array[], const Test value, const Range range, const Comparison compare, const size_t unique) {
	size_t skip = Range_length(range)/unique, index = range.end - skip;
	while (index > range.start && !compare(array[index - 1], value)) {
		if (index < range.start + skip) {
			skip = index - range.start;
			index = range.start;
			break;
		}
		index -= skip;
	}
	
	return BinaryFirst(array, value, Range_new(index, index + skip), compare);
}

size_t FindLastBackward(const Test array[], const Test value, const Range range, const Comparison compare, const size_t unique) {
	size_t skip = Range_length(range)/unique, index = range.end - skip;
	while (index > range.start && compare(value, array[index - 1])) {
		if (index < range.start + skip) {
			skip = index - range.start;
			index = range.start;
			break;
		}
		index -= skip;
	}
	
	return BinaryLast(array, value, Range_new(index, index + skip), compare);
}

/* n^2 sorting algorithm used to sort tiny chunks of the full array */
void InsertionSort(Test array[], const Range range, const Comparison compare) {
	size_t i, j;
	for (i = range.start + 1; i < range.end; i++) {
		const Test temp = array[i];
		for (j = i; j > range.start && compare(temp, array[j - 1]); j--)
			array[j] = array[j - 1];
		array[j] = temp;
	}
}

/* binary search variant of insertion sort, */
/* which reduces the number of comparisons at the cost of some speed */
void InsertionSortBinary(Test array[], const Range range, const Comparison compare) {
	size_t i, j, insert;
	for (i = range.start + 1; i < range.end; i++) {
		const Test temp = array[i];
		insert = BinaryLast(array, temp, Range_new(range.start, i), compare);
		for (j = i; j > insert; j--)
			array[j] = array[j - 1];
		array[insert] = temp;
	}
}

/* reverse a range within the array */
void Reverse(Test array[], const Range range) {
	size_t index;
	for (index = Range_length(range)/2; index > 0; index--)
		Swap(array[range.start + index - 1], array[range.end - index]);
}

/* swap a series of values in the array */
void BlockSwap(Test array[], const size_t start1, const size_t start2, const size_t block_size) {
	size_t index;
	for (index = 0; index < block_size; index++)
		Swap(array[start1 + index], array[start2 + index]);
}

/* rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1) */
/* (the GCD variant of this was tested, but despite having fewer assignments it was never faster than three reversals!) */
void Rotate(Test array[], const size_t amount, const Range range, Test cache[], const size_t cache_size) {
	size_t split; Range range1, range2;
	if (Range_length(range) == 0) return;
	
	split = range.start + amount;
	range1 = Range_new(range.start, split);
	range2 = Range_new(split, range.end);
	
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

/* calculate how to scale the index value to the range within the array */
/* this is essentially 64.64 fixed-point math, where we manually check for and handle overflow, */
/* and where the fractional part is in base "fractional_base", rather than base 10 */
typedef struct {
	size_t size, power_of_two;
	size_t fractional, decimal;
	size_t fractional_base, decimal_step, fractional_step;
} WikiIterator;

void WikiIterator_begin(WikiIterator *me) {
	me->fractional = me->decimal = 0;
}

Range WikiIterator_nextRange(WikiIterator *me) {
	size_t start = me->decimal;
	
	me->decimal += me->decimal_step;
	me->fractional += me->fractional_step;
	if (me->fractional >= me->fractional_base) {
		me->fractional -= me->fractional_base;
		me->decimal++;
	}
	
	return Range_new(start, me->decimal);
}

bool WikiIterator_finished(WikiIterator *me) {
	return (me->decimal >= me->size);
}

bool WikiIterator_nextLevel(WikiIterator *me) {
	me->decimal_step += me->decimal_step;
	me->fractional_step += me->fractional_step;
	if (me->fractional_step >= me->fractional_base) {
		me->fractional_step -= me->fractional_base;
		me->decimal_step++;
	}
	
	return (me->decimal_step < me->size);
}

size_t WikiIterator_length(WikiIterator *me) {
	return me->decimal_step;
}

WikiIterator WikiIterator_new(size_t size2, size_t min_level) {
	WikiIterator me;
	me.size = size2;
	me.power_of_two = FloorPowerOfTwo(me.size);
	me.fractional_base = me.power_of_two/min_level;
	me.fractional_step = me.size % me.fractional_base;
	me.decimal_step = me.size/me.fractional_base;
	WikiIterator_begin(&me);
	return me;
}

/* merge operation using an external buffer, */
void MergeExternal(Test array[], const Range A, const Range B, const Comparison compare, Test cache[], const size_t cache_size) {
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
	size_t A_count = 0, B_count = 0, insert = 0;
	
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
void MergeInPlace(Test array[], Range A, Range B, const Comparison compare, Test cache[], const size_t cache_size) {
	/*
	 this just repeatedly binary searches into B and rotates A into position.
	 the paper suggests using the 'rotation-based Hwang and Lin algorithm' here,
	 but I decided to stick with this because it had better situational performance
	 
	 normally this is incredibly suboptimal, but this function is only called
	 when none of the A or B blocks in any subarray contained 2√A unique values,
	 which places a hard limit on the number of times this will ACTUALLY need
	 to binary search and rotate.
	 
	 according to my analysis the worst case is √A rotations performed on √A items
	 once the constant factors are removed, which ends up being O(n)
	 
	 again, this is NOT a general-purpose solution – it only works well in this case!
	 kind of like how the O(n^2) insertion sort is used in some places
	 */
	
	while (Range_length(A) > 0 && Range_length(B) > 0) {
		/* find the first place in B where the first item in A needs to be inserted */
		size_t mid = BinaryFirst(array, array[A.start], B, compare);
		
		/* rotate A into place */
		size_t amount = mid - A.end;
		Rotate(array, Range_length(A), Range_new(A.start, mid), cache, cache_size);
		
		/* calculate the new A and B ranges */
		B.start = mid;
		A = Range_new(BinaryLast(array, array[A.start + amount], A, compare), B.start);
	}
}

/* bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use */
void WikiSort(Test array[], const size_t size, const Comparison compare) {
	/* use a small cache to speed up some of the operations.
	 since the cache size is fixed, it's still O(1) memory!
	 just keep in mind that making it too small ruins the point (nothing will fit into it)
	 and making it too large also ruins the point (so much for "low memory"!)
	 removing the cache entirely still gives 70% of the performance of a standard merge */
	
	#define CACHE_SIZE 512
	const size_t cache_size = CACHE_SIZE;
	Test cache[CACHE_SIZE];
	
	/* note that you can easily modify the above to allocate a dynamically sized cache
	 good choices for the cache size are:
	 (size + 1)/2 – turns into a full-speed standard merge sort since everything fits into the cache
	 sqrt((size + 1)/2) + 1 – this will be the size of the A blocks at the largest level of merges,
	 so a buffer of this size would allow it to skip using internal or in-place merges for anything
	 512 – chosen from careful testing as a good balance between fixed-size memory use and run time
	 0 – if the system simply cannot allocate any extra memory whatsoever, no memory works just fine */
	
	WikiIterator iterator;
	
	/* if there are 32 or fewer items, just insertion sort the entire array */
	if (size <= 32) {
		InsertionSort(array, Range_new(0, size), compare);
		return;
	}
	
	/* first insertion sort everything the lowest level, which is 8-15 items at a time */
	iterator = WikiIterator_new(size, 8);
	WikiIterator_begin(&iterator);
	while (!WikiIterator_finished(&iterator))
		InsertionSortBinary(array, WikiIterator_nextRange(&iterator), compare);
	
	/* then merge sort the higher levels, which can be 16-31, 32-63, 64-127, 128-255, etc. */
	while (true) {
		
		/* if every A and B block will fit into the cache, use a special branch specifically for merging with the cache */
		/* (we use < rather than <= since the block size might be one more than decimal_step) */
		if (WikiIterator_length(&iterator) < cache_size) {
			WikiIterator_begin(&iterator);
			while (!WikiIterator_finished(&iterator)) {
				Range A = WikiIterator_nextRange(&iterator);
				Range B = WikiIterator_nextRange(&iterator);
				
				if (compare(array[B.start], array[A.end - 1])) {
					/* these two ranges weren't already in order, so we'll need to merge them! */
					memcpy(&cache[0], &array[A.start], Range_length(A) * sizeof(array[0]));
					MergeExternal(array, A, B, compare, cache, cache_size);
				}
			}
		} else {
			/* this is where the in-place merge logic starts!
			 1. pull out two internal buffers each containing √A unique values
				1a. adjust block_size and buffer_size if we couldn't find enough unique values
			 2. loop over the A and B areas within this level of the merge sort
			 3. break A and B into blocks of size 'block_size'
			 4. "tag" each of the A blocks with values from the first internal buffer
			 5. roll the A blocks through the B blocks and drop/rotate them where they belong
			 6. merge each A block with any B values that follow, using the cache or second the internal buffer
			 7. sort the second internal buffer if it exists
			 8. redistribute the two internal buffers back into the array */
			
			size_t block_size = sqrt(WikiIterator_length(&iterator));
			size_t buffer_size = WikiIterator_length(&iterator)/block_size + 1;
			
			/* as an optimization, we really only need to pull out the internal buffers once for each level of merges */
			/* after that we can reuse the same buffers over and over, then redistribute it when we're finished with this level */
			Range buffer1, buffer2, A, B;
			size_t index, last, count, find, pull_index = 0;
			struct { size_t from, to, count; Range range; } pull[2] = { { 0 }, { 0 } };
			
			buffer1 = Range_new(0, 0);
			buffer2 = Range_new(0, 0);
			
			/* if every A block fits into the cache, we don't need the second internal buffer, so we can make do with only 'buffer_size' unique values */
			find = buffer_size + buffer_size;
			if (block_size <= cache_size) find = buffer_size;
			
			/* we need to find either a single contiguous space containing 2√A unique values (which will be split up into two buffers of size √A each), */
			/* or we need to find one buffer of < 2√A unique values, and a second buffer of √A unique values, */
			/* OR if we couldn't find that many unique values, we need the largest possible buffer we can get */
			
			/* in the case where it couldn't find a single buffer of at least √A unique values, */
			/* all of the Merge steps must be replaced by a different merge algorithm (MergeInPlace) */
			WikiIterator_begin(&iterator);
			while (!WikiIterator_finished(&iterator)) {
				A = WikiIterator_nextRange(&iterator);
				B = WikiIterator_nextRange(&iterator);
				
				/* check A for the number of unique values we need to fill an internal buffer */
				/* these values will be pulled out to the start of A */
				last = A.start;
				count = 1;
				/* assume find is > 1 */
				while (true) {
					index = FindLastForward(array, array[last], Range_new(last + 1, A.end), compare, find - count);
					if (index == A.end) break;
					last = index;
					if (++count >= find) break;
				}
				index = last;
				
				if (count >= buffer_size) {
					/* keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer */
					pull[pull_index].range = Range_new(A.start, B.end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = A.start;
					pull_index = 1;
					
					if (count == buffer_size + buffer_size) {
						/* we were able to find a single contiguous section containing 2√A unique values, */
						/* so this section can be used to contain both of the internal buffers we'll need */
						buffer1 = Range_new(A.start, A.start + buffer_size);
						buffer2 = Range_new(A.start + buffer_size, A.start + count);
						break;
					} else if (find == buffer_size + buffer_size) {
						buffer1 = Range_new(A.start, A.start + count);
						
						/* we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values, */
						/* so we still need to find a second separate buffer of at least √A unique values */
						find = buffer_size;
					} else if (block_size <= cache_size) {
						/* we found the first and only internal buffer that we need, so we're done! */
						buffer1 = Range_new(A.start, A.start + count);
						break;
					} else {
						/* we found a second buffer in an 'A' area containing √A unique values, so we're done! */
						buffer2 = Range_new(A.start, A.start + count);
						break;
					}
				} else if (pull_index == 0 && count > Range_length(buffer1)) {
					/* keep track of the largest buffer we were able to find */
					buffer1 = Range_new(A.start, A.start + count);
					
					pull[pull_index].range = Range_new(A.start, B.end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = A.start;
				}
				
				/* check B for the number of unique values we need to fill an internal buffer */
				/* these values will be pulled out to the end of B */
				last = B.end - 1;
				count = 1;
				while (true) {
					index = FindFirstBackward(array, array[last], Range_new(B.start, last), compare, find - count);
					if (index == B.start) break;
					last = index - 1;
					if (++count >= find) break;
				}
				index = last;
				
				if (count >= buffer_size) {
					/* keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer */
					pull[pull_index].range = Range_new(A.start, B.end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = B.end;
					pull_index = 1;
					
					if (count == buffer_size + buffer_size) {
						/* we were able to find a single contiguous section containing 2√A unique values, */
						/* so this section can be used to contain both of the internal buffers we'll need */
						buffer1 = Range_new(B.end - count, B.end - buffer_size);
						buffer2 = Range_new(B.end - buffer_size, B.end);
						break;
					} else if (find == buffer_size + buffer_size) {
						buffer1 = Range_new(B.end - count, B.end);
						
						/* we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values, */
						/* so we still need to find a second separate buffer of at least √A unique values */
						find = buffer_size;
					} else if (block_size <= cache_size) {
						/* we found the first and only internal buffer that we need, so we're done! */
						buffer1 = Range_new(B.end - count, B.end);
						break;
					} else {
						/* we found a second buffer in an 'B' area containing √A unique values, so we're done! */
						buffer2 = Range_new(B.end - count, B.end);
						
						/* buffer2 will be pulled out from a 'B' area, so if the first buffer was pulled out from the corresponding 'A' area, */
						/* we need to adjust the end point for that A area so it knows to stop redistributing its values before reaching buffer2 */
						if (pull[0].range.start == A.start) pull[0].range.end -= pull[1].count;
						
						break;
					}
				} else if (pull_index == 0 && count > Range_length(buffer1)) {
					/* keep track of the largest buffer we were able to find */
					buffer1 = Range_new(B.end - count, B.end);
					
					pull[pull_index].range = Range_new(A.start, B.end);
					pull[pull_index].count = count;
					pull[pull_index].from = index;
					pull[pull_index].to = B.end;
				}
			}
			
			/* pull out the two ranges so we can use them as internal buffers */
			for (pull_index = 0; pull_index < 2; pull_index++) {
				Range range;
				size_t length = pull[pull_index].count;
				count = 0;
				
				if (pull[pull_index].to < pull[pull_index].from) {
					/* we're pulling the values out to the left, which means the start of an A area */
					count = 1;
					index = pull[pull_index].from;
					while (count < length) {
						index = FindFirstBackward(array, array[index - 1], Range_new(pull[pull_index].to, pull[pull_index].from - (count - 1)), compare, length - count);
						range = Range_new(index + 1, pull[pull_index].from + 1);
						Rotate(array, Range_length(range) - count, range, cache, cache_size);
						pull[pull_index].from = index + count;
						count++;
					}
				} else if (pull[pull_index].to > pull[pull_index].from) {
					/* we're pulling values out to the right, which means the end of a B area */
					count = 1;
					index = pull[pull_index].from + count;
					while (count < length) {
						index = FindLastForward(array, array[index], Range_new(index, pull[pull_index].to), compare, length - count);
						range = Range_new(pull[pull_index].from, index - 1);
						Rotate(array, count, range, cache, cache_size);
						pull[pull_index].from = index - 1 - count;
						count++;
					}
				}
			}
			
			/* adjust block_size and buffer_size based on the values we were able to pull out */
			buffer_size = Range_length(buffer1);
			block_size = WikiIterator_length(&iterator)/buffer_size + 1;
			
			/* the first buffer NEEDS to be large enough to tag each of the evenly sized A blocks, */
			/* so this was originally here to test the math for adjusting block_size above */
			/* assert((WikiIterator_length(&iterator) + 1)/block_size <= buffer_size); */
			
			/* now that the two internal buffers have been created, it's time to merge each A+B combination at this level of the merge sort! */
			WikiIterator_begin(&iterator);
			while (!WikiIterator_finished(&iterator)) {
				A = WikiIterator_nextRange(&iterator);
				B = WikiIterator_nextRange(&iterator);
				
				/* remove any parts of A or B that are being used by the internal buffers */
				find = A.start;
				for (pull_index = 0; pull_index < 2; pull_index++) {
					if (find == pull[pull_index].range.start) {
						if (pull[pull_index].from > pull[pull_index].to)
							A.start += pull[pull_index].count;
						else if (pull[pull_index].from < pull[pull_index].to)
							B.end -= pull[pull_index].count;
					}
				}
				
				if (compare(array[A.end], array[A.end - 1])) {
					/* these two ranges weren't already in order, so we'll need to merge them! */
					Range blockA, firstA, lastA, lastB, blockB;
					size_t minA, indexA, findA;
					__typeof__(array[0]) min_value;
					
					/* break the remainder of A into blocks. firstA is the uneven-sized first A block */
					blockA = Range_new(A.start, A.end);
					firstA = Range_new(A.start, A.start + Range_length(blockA) % block_size);
					
					/* swap the second value of each A block with the value in buffer1 */
					for (index = 0, indexA = firstA.end + 1; indexA < blockA.end; index++, indexA += block_size) 
						Swap(array[buffer1.start + index], array[indexA]);
					
					/* start rolling the A blocks through the B blocks! */
					/* whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well */
					lastA = firstA;
					lastB = Range_new(0, 0);
					blockB = Range_new(B.start, B.start + Min(block_size, Range_length(B)));
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
							size_t B_split = BinaryFirst(array, min_value, lastB, compare);
							size_t B_remaining = lastB.end - B_split;
							
							/* swap the minimum A block to the beginning of the rolling A blocks */
							BlockSwap(array, blockA.start, minA, block_size);
							
							/* swap the second item of the previous A block back with its original value, which is stored in buffer1 */
							Swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
							
							/*
							 locally merge the previous A block with the B values that follow it
							 if lastA fits into the external cache we'll use that (with MergeExternal),
							 or if the second internal buffer exists we'll use that (with MergeInternal),
							 or failing that we'll use a strictly in-place merge algorithm (MergeInPlace)
							 */
							if (Range_length(lastA) <= cache_size)
								MergeExternal(array, lastA, Range_new(lastA.end, B_split), compare, cache, cache_size);
							else if (Range_length(buffer2) > 0)
								MergeInternal(array, lastA, Range_new(lastA.end, B_split), compare, buffer2);
							else
								MergeInPlace(array, lastA, Range_new(lastA.end, B_split), compare, cache, cache_size);
							
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
								Rotate(array, blockA.start - B_split, Range_new(B_split, blockA.start + block_size), cache, cache_size);
							}
							
							/* update the range for the remaining A blocks, and the range remaining from the B block after it was split */
							lastA = Range_new(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
							lastB = Range_new(lastA.end, lastA.end + B_remaining);
							
							/* if there are no more A blocks remaining, this step is finished! */
							blockA.start += block_size;
							if (Range_length(blockA) == 0)
								break;
							
							/* search the second value of the remaining A blocks to find the new minimum A block */
							minA = blockA.start;
							for (findA = minA + block_size; findA < blockA.end; findA += block_size)
								if (compare(array[findA + 1], array[minA + 1]))
									minA = findA;
							min_value = array[minA];
							
						} else if (Range_length(blockB) < block_size) {
							/* move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation */
							/* the cache is disabled here since it might contain the contents of the previous A block */
							Rotate(array, blockB.start - blockA.start, Range_new(blockA.start, blockB.end), cache, 0);
							
							lastB = Range_new(blockA.start, blockA.start + Range_length(blockB));
							blockA.start += Range_length(blockB);
							blockA.end += Range_length(blockB);
							minA += Range_length(blockB);
							blockB.end = blockB.start;
						} else {
							/* roll the leftmost A block to the end by swapping it with the next B block */
							BlockSwap(array, blockA.start, blockB.start, block_size);
							lastB = Range_new(blockA.start, blockA.start + block_size);
							if (minA == blockA.start)
								minA = blockA.end;
							
							blockA.start += block_size;
							blockA.end += block_size;
							blockB.start += block_size;
							
							if (blockB.end > B.end - block_size) blockB.end = B.end;
							else blockB.end += block_size;
						}
					}
					
					/* merge the last A block with the remaining B values */
					if (Range_length(lastA) <= cache_size)
						MergeExternal(array, lastA, Range_new(lastA.end, B.end), compare, cache, cache_size);
					else if (Range_length(buffer2) > 0)
						MergeInternal(array, lastA, Range_new(lastA.end, B.end), compare, buffer2);
					else
						MergeInPlace(array, lastA, Range_new(lastA.end, B.end), compare, cache, cache_size);
				} else if (compare(array[B.end - 1], array[A.start])) {
					/* the two ranges are in reverse order, so a simple rotation should fix it */
					Rotate(array, A.end - A.start, Range_new(A.start, B.end), cache, cache_size);
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
					Range buffer = Range_new(pull[pull_index].range.start, pull[pull_index].range.start + pull[pull_index].count);
					size_t amount, unique = Range_length(buffer) * 2;
					while (Range_length(buffer) > 0) {
						index = FindFirstForward(array, array[buffer.start], Range_new(buffer.end, pull[pull_index].range.end), compare, unique);
						amount = index - buffer.end;
						Rotate(array, Range_length(buffer), Range_new(buffer.start, index), cache, cache_size);
						buffer.start += (amount + 1);
						buffer.end += amount;
						unique -= 2;
					}
				} else if (pull[pull_index].from < pull[pull_index].to) {
					/* the values were pulled out to the right, so redistribute them back to the left */
					Range buffer = Range_new(pull[pull_index].range.end - pull[pull_index].count, pull[pull_index].range.end);
					size_t amount, unique = Range_length(buffer) * 2;
					while (Range_length(buffer) > 0) {
						index = FindLastBackward(array, array[buffer.end - 1], Range_new(pull[pull_index].range.start, buffer.start), compare, unique);
						amount = buffer.start - index;
						Rotate(array, amount, Range_new(index, buffer.end), cache, cache_size);
						buffer.start -= amount;
						buffer.end -= (amount + 1);
						unique -= 2;
					}
				}
			}
		}
		
		/* double the size of each A and B area that will be merged in the next level */
		if (!WikiIterator_nextLevel(&iterator)) break;
	}
	
	#undef CACHE_SIZE
}




/* standard merge sort, so we have a baseline for how well WikiSort works */
void MergeSortR(Test array[], const Range range, const Comparison compare, Test buffer[]) {
	size_t mid, A_count = 0, B_count = 0, insert = 0;
	Range A, B;
	
	if (Range_length(range) < 32) {
		InsertionSort(array, range, compare);
		return;
	}
	
	mid = range.start + (range.end - range.start)/2;
	A = Range_new(range.start, mid);
	B = Range_new(mid, range.end);
	
	MergeSortR(array, A, compare, buffer);
	MergeSortR(array, B, compare, buffer);
	
	/* standard merge operation here (only A is copied to the buffer, and only the parts that weren't already where they should be) */
	A = Range_new(BinaryLast(array, array[B.start], A, compare), A.end);
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

void MergeSort(Test array[], const size_t array_count, const Comparison compare) {
	Var(buffer, Allocate(Test, (array_count + 1)/2));
	MergeSortR(array, Range_new(0, array_count), compare, buffer);
	free(buffer);
}




size_t TestingRandom(size_t index, size_t total) {
	return rand();
}

size_t TestingRandomFew(size_t index, size_t total) {
	return rand() * (100.0/RAND_MAX);
}

size_t TestingMostlyDescending(size_t index, size_t total) {
	return total - index + rand() * 1.0/RAND_MAX * 5 - 2.5;
}

size_t TestingMostlyAscending(size_t index, size_t total) {
	return index + rand() * 1.0/RAND_MAX * 5 - 2.5;
}

size_t TestingAscending(size_t index, size_t total) {
	return index;
}

size_t TestingDescending(size_t index, size_t total) {
	return total - index;
}

size_t TestingEqual(size_t index, size_t total) {
	return 1000;
}

size_t TestingJittered(size_t index, size_t total) {
	return (rand() * 1.0/RAND_MAX <= 0.9) ? index : (index - 2);
}

size_t TestingMostlyEqual(size_t index, size_t total) {
	return 1000 + rand() * 1.0/RAND_MAX * 4;
}


/* make sure the items within the given range are in a stable order */
/* if you want to test the correctness of any changes you make to the main WikiSort function,
 move this function to the top of the file and call it from within WikiSort after each step */
void WikiVerify(const Test array[], const Range range, const Comparison compare, const char *msg) {
	size_t index;
	for (index = range.start + 1; index < range.end; index++) {
		/* if it's in ascending order then we're good */
		/* if both values are equal, we need to make sure the index values are ascending */
		if (!(compare(array[index - 1], array[index]) ||
			  (!compare(array[index], array[index - 1]) && array[index].index > array[index - 1].index))) {
			
			/*for (index2 = range.start; index2 < range.end; index2++) */
			/*	printf("%lu (%lu) ", array[index2].value, array[index2].index); */
			
			printf("failed with message: %s\n", msg);
			assert(false);
		}
	}
}

int main() {
	size_t total, index;
	double total_time, total_time1, total_time2;
	const size_t max_size = 1500000;
	Var(array1, Allocate(Test, max_size));
	Var(array2, Allocate(Test, max_size));
	Comparison compare = TestCompare;
	
	#if PROFILE
		size_t compares1, compares2, total_compares1 = 0, total_compares2 = 0;
	#endif
	#if !SLOW_COMPARISONS
		size_t test_case;
		__typeof__(&TestingRandom) test_cases[] = {
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
	#endif
	
	/* initialize the random-number generator */
	srand(time(NULL));
	/*srand(10141985);*/ /* in case you want the same random numbers */
	
	total = max_size;
	
#if !SLOW_COMPARISONS
	printf("running test cases... ");
	fflush(stdout);
	
	for (test_case = 0; test_case < sizeof(test_cases)/sizeof(test_cases[0]); test_case++) {
		
		for (index = 0; index < total; index++) {
			Test item;
			
			item.value = test_cases[test_case](index, total);
			item.index = index;
			
			array1[index] = array2[index] = item;
		}
		
		WikiSort(array1, total, compare);
		
		MergeSort(array2, total, compare);
		
		WikiVerify(array1, Range_new(0, total), compare, "test case failed");
		for (index = 0; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
	}
	printf("passed!\n");
#endif
	
	total_time = Seconds();
	total_time1 = total_time2 = 0;
	
	for (total = 0; total < max_size; total += 2048 * 16) {
		double time1, time2;
		
		for (index = 0; index < total; index++) {
			Test item;
			
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
		#if PROFILE
			comparisons = 0;
		#endif
		WikiSort(array1, total, compare);
		time1 = Seconds() - time1;
		total_time1 += time1;
		#if PROFILE
			compares1 = comparisons;
			total_compares1 += compares1;
		#endif
		
		time2 = Seconds();
		#if PROFILE
			comparisons = 0;
		#endif
		MergeSort(array2, total, compare);
		time2 = Seconds() - time2;
		total_time2 += time2;
		#if PROFILE
			compares2 = comparisons;
			total_compares2 += compares2;
		#endif
		
		if (time1 >= time2)
			printf("[%zu] WikiSort: %.2f seconds, MergeSort: %.2f seconds (%.2f%% as fast)\n", total, time1, time2, time2/time1 * 100.0);
		else
			printf("[%zu] WikiSort: %.2f seconds, MergeSort: %.2f seconds (%.2f%% faster)\n", total, time1, time2, time2/time1 * 100.0 - 100.0);
		
		#if PROFILE
			if (compares1 <= compares2)
				printf("[%zu] WikiSort: %zu compares, MergeSort: %zu compares (%.2f%% as many)\n", total, compares1, compares2, compares1 * 100.0/compares2);
			else
				printf("[%zu] WikiSort: %zu compares, MergeSort: %zu compares (%.2f%% more)\n", total, compares1, compares2, compares1 * 100.0/compares2 - 100.0);
		#endif
		
		/* make sure the arrays are sorted correctly, and that the results were stable */
		printf("verifying... ");
		fflush(stdout);
		
		WikiVerify(array1, Range_new(0, total), compare, "testing the final array");
		for (index = 0; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		
		printf("correct!\n");
	}
	
	total_time = Seconds() - total_time;
	printf("tests completed in %.2f seconds\n", total_time);
	if (total_time1 >= total_time2)
		printf("WikiSort: %.2f seconds, MergeSort: %.2f seconds (%.2f%% as fast)\n", total_time1, total_time2, total_time2/total_time1 * 100.0);
	else
		printf("WikiSort: %.2f seconds, MergeSort: %.2f seconds (%.2f%% faster)\n", total_time1, total_time2, total_time2/total_time1 * 100.0 - 100.0);
	
	#if PROFILE
		if (total_compares1 <= total_compares2)
			printf("WikiSort: %zu compares, MergeSort: %zu compares (%.2f%% as many)\n", total_compares1, total_compares2, total_compares1 * 100.0/total_compares2);
		else
			printf("WikiSort: %zu compares, MergeSort: %zu compares (%.2f%% more)\n", total_compares1, total_compares2, total_compares1 * 100.0/total_compares2 - 100.0);
	#endif
	
	free(array1); free(array2);
	return 0;
}
