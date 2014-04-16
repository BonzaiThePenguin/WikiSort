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
/* note that this reduces WikiSort's performance when enabled */
#define PROFILE false

/* verify that WikiSort is actually correct */
/* (this also reduces performance slightly) */
#define VERIFY false

/* simulate comparisons that have a bit more overhead than just an inlined (int < int) */
/* (so we can tell whether reducing the number of comparisons was worth the added complexity) */
#define SLOW_COMPARISONS false

/* whether to give WikiSort a full-size cache, to see how it performs when given more memory */
#define DYNAMIC_CACHE false


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
#if VERIFY
	size_t index;
#endif
} Test;

#if PROFILE
	/* global for testing how many comparisons are performed for each sorting algorithm */
	size_t comparisons;
#endif

#if SLOW_COMPARISONS
	#define NOOP_SIZE 50
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
	size_t skip, index;
	if (Range_length(range) == 0) return range.start;
	skip = Max(Range_length(range)/unique, 1);
	
	for (index = range.start + skip; compare(array[index - 1], value); index += skip)
		if (index >= range.end - skip)
			return BinaryFirst(array, value, Range_new(index, range.end), compare);
	
	return BinaryFirst(array, value, Range_new(index - skip, index), compare);
}

size_t FindLastForward(const Test array[], const Test value, const Range range, const Comparison compare, const size_t unique) {
	size_t skip, index;
	if (Range_length(range) == 0) return range.start;
	skip = Max(Range_length(range)/unique, 1);
	
	for (index = range.start + skip; !compare(value, array[index - 1]); index += skip)
		if (index >= range.end - skip)
			return BinaryLast(array, value, Range_new(index, range.end), compare);
	
	return BinaryLast(array, value, Range_new(index - skip, index), compare);
}

size_t FindFirstBackward(const Test array[], const Test value, const Range range, const Comparison compare, const size_t unique) {
	size_t skip, index;
	if (Range_length(range) == 0) return range.start;
	skip = Max(Range_length(range)/unique, 1);
	
	for (index = range.end - skip; index > range.start && !compare(array[index - 1], value); index -= skip)
		if (index < range.start + skip)
			return BinaryFirst(array, value, Range_new(range.start, index), compare);
	
	return BinaryFirst(array, value, Range_new(index, index + skip), compare);
}

size_t FindLastBackward(const Test array[], const Test value, const Range range, const Comparison compare, const size_t unique) {
	size_t skip, index;
	if (Range_length(range) == 0) return range.start;
	skip = Max(Range_length(range)/unique, 1);
	
	for (index = range.end - skip; index > range.start && compare(value, array[index - 1]); index -= skip)
		if (index < range.start + skip)
			return BinaryLast(array, value, Range_new(range.start, index), compare);
	
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

/* reverse a range of values within the array */
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
/* this assumes that 0 <= amount <= range.length() */
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
/* the bottom-up merge sort only operates on values that are powers of two, */
/* so scale down to that power of two, then use a fraction to scale back again */
typedef struct {
	size_t size, power_of_two;
	size_t numerator, decimal;
	size_t denominator, decimal_step, numerator_step;
} WikiIterator;

void WikiIterator_begin(WikiIterator *me) {
	me->numerator = me->decimal = 0;
}

Range WikiIterator_nextRange(WikiIterator *me) {
	size_t start = me->decimal;
	
	me->decimal += me->decimal_step;
	me->numerator += me->numerator_step;
	if (me->numerator >= me->denominator) {
		me->numerator -= me->denominator;
		me->decimal++;
	}
	
	return Range_new(start, me->decimal);
}

bool WikiIterator_finished(WikiIterator *me) {
	return (me->decimal >= me->size);
}

bool WikiIterator_nextLevel(WikiIterator *me) {
	me->decimal_step += me->decimal_step;
	me->numerator_step += me->numerator_step;
	if (me->numerator_step >= me->denominator) {
		me->numerator_step -= me->denominator;
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
	me.denominator = me.power_of_two/min_level;
	me.numerator_step = me.size % me.denominator;
	me.decimal_step = me.size/me.denominator;
	WikiIterator_begin(&me);
	return me;
}

/* merge two ranges from one array and save the results into a different array */
void MergeInto(Test from[], const Range A, const Range B, const Comparison compare, Test into[]) {
	Test *A_index = &from[A.start], *B_index = &from[B.start];
	Test *A_last = &from[A.end], *B_last = &from[B.end];
	Test *insert_index = &into[0];
	
	while (true) {
		if (!compare(*B_index, *A_index)) {
			*insert_index = *A_index;
			A_index++;
			insert_index++;
			if (A_index == A_last) {
				/* copy the remainder of B into the final array */
				memcpy(insert_index, B_index, (B_last - B_index) * sizeof(from[0]));
				break;
			}
		} else {
			*insert_index = *B_index;
			B_index++;
			insert_index++;
			if (B_index == B_last) {
				/* copy the remainder of A into the final array */
				memcpy(insert_index, A_index, (A_last - A_index) * sizeof(from[0]));
				break;
			}
		}
	}
}

/* merge operation using an external buffer, */
void MergeExternal(Test array[], const Range A, const Range B, const Comparison compare, Test cache[]) {
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
	if (Range_length(A) == 0 || Range_length(B) == 0) return;
	
	/*
	 this just repeatedly binary searches into B and rotates A into position.
	 the paper suggests using the 'rotation-based Hwang and Lin algorithm' here,
	 but I decided to stick with this because it had better situational performance
	 
	 (Hwang and Lin is designed for merging subarrays of very different sizes,
	 but WikiSort almost always uses subarrays that are roughly the same size)
	 
	 normally this is incredibly suboptimal, but this function is only called
	 when none of the A or B blocks in any subarray contained 2√A unique values,
	 which places a hard limit on the number of times this will ACTUALLY need
	 to binary search and rotate.
	 
	 according to my analysis the worst case is √A rotations performed on √A items
	 once the constant factors are removed, which ends up being O(n)
	 
	 again, this is NOT a general-purpose solution – it only works well in this case!
	 kind of like how the O(n^2) insertion sort is used in some places
	 */
	
	while (true) {
		/* find the first place in B where the first item in A needs to be inserted */
		size_t mid = BinaryFirst(array, array[A.start], B, compare);
		
		/* rotate A into place */
		size_t amount = mid - A.end;
		Rotate(array, Range_length(A), Range_new(A.start, mid), cache, cache_size);
		if (B.end == mid) break;
		
		/* calculate the new A and B ranges */
		B.start = mid;
		A = Range_new(A.start + amount, B.start);
		A.start = BinaryLast(array, array[A.start], A, compare);
		if (Range_length(A) == 0) break;
	}
}

/* bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use */
void WikiSort(Test array[], const size_t size, const Comparison compare) {
	/* use a small cache to speed up some of the operations */
	#if DYNAMIC_CACHE
		size_t cache_size;
		Test *cache = NULL;
	#else
		/* since the cache size is fixed, it's still O(1) memory! */
		/* just keep in mind that making it too small ruins the point (nothing will fit into it), */
		/* and making it too large also ruins the point (so much for "low memory"!) */
		/* removing the cache entirely still gives 70% of the performance of a standard merge */
		#define CACHE_SIZE 512
		const size_t cache_size = CACHE_SIZE;
		Test cache[CACHE_SIZE];
	#endif
	
	WikiIterator iterator;
	
	/* if the array is of size 0, 1, 2, or 3, just sort them like so: */
	if (size < 4) {
		if (size == 3) {
			/* hard-coded insertion sort */
			if (compare(array[1], array[0])) Swap(array[0], array[1]);
			if (compare(array[2], array[1])) {
				Swap(array[1], array[2]);
				if (compare(array[1], array[0])) Swap(array[0], array[1]);
			}
		} else if (size == 2) {
			/* swap the items if they're out of order */
			if (compare(array[1], array[0])) Swap(array[0], array[1]);
		}
		
		return;
	}
	
	/* sort groups of 4-8 items at a time using an unstable sorting network, */
	/* but keep track of the original item orders to force it to be stable */
	/* http://pages.ripco.net/~jgamble/nw.html */
	iterator = WikiIterator_new(size, 4);
	WikiIterator_begin(&iterator);
	while (!WikiIterator_finished(&iterator)) {
		uint8_t order[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
		Range range = WikiIterator_nextRange(&iterator);
		
		#define SWAP(x, y) \
			if (compare(array[range.start + y], array[range.start + x]) || \
				(order[x] > order[y] && !compare(array[range.start + x], array[range.start + y]))) { \
				Swap(array[range.start + x], array[range.start + y]); Swap(order[x], order[y]); }
		
		if (Range_length(range) == 8) {
			SWAP(0, 1); SWAP(2, 3); SWAP(4, 5); SWAP(6, 7);
			SWAP(0, 2); SWAP(1, 3); SWAP(4, 6); SWAP(5, 7);
			SWAP(1, 2); SWAP(5, 6); SWAP(0, 4); SWAP(3, 7);
			SWAP(1, 5); SWAP(2, 6);
			SWAP(1, 4); SWAP(3, 6);
			SWAP(2, 4); SWAP(3, 5);
			SWAP(3, 4);
			
		} else if (Range_length(range) == 7) {
			SWAP(1, 2); SWAP(3, 4); SWAP(5, 6);
			SWAP(0, 2); SWAP(3, 5); SWAP(4, 6);
			SWAP(0, 1); SWAP(4, 5); SWAP(2, 6);
			SWAP(0, 4); SWAP(1, 5);
			SWAP(0, 3); SWAP(2, 5);
			SWAP(1, 3); SWAP(2, 4);
			SWAP(2, 3);
			
		} else if (Range_length(range) == 6) {
			SWAP(1, 2); SWAP(4, 5);
			SWAP(0, 2); SWAP(3, 5);
			SWAP(0, 1); SWAP(3, 4); SWAP(2, 5);
			SWAP(0, 3); SWAP(1, 4);
			SWAP(2, 4); SWAP(1, 3);
			SWAP(2, 3);
			
		} else if (Range_length(range) == 5) {
			SWAP(0, 1); SWAP(3, 4);
			SWAP(2, 4);
			SWAP(2, 3); SWAP(1, 4);
			SWAP(0, 3);
			SWAP(0, 2); SWAP(1, 3);
			SWAP(1, 2);
			
		} else if (Range_length(range) == 4) {
			SWAP(0, 1); SWAP(2, 3);
			SWAP(0, 2); SWAP(1, 3);
			SWAP(1, 2);
			
		}
	}
	if (size < 8) return;
	
	
	#if DYNAMIC_CACHE
		/* good choices for the cache size are: */
		/* (size + 1)/2 – turns into a full-speed standard merge sort since everything fits into the cache */
		cache_size = (size + 1)/2;
		cache = (Test *)malloc(cache_size * sizeof(array[0]));
		
		if (!cache) {
			/* sqrt((size + 1)/2) + 1 – this will be the size of the A blocks at the largest level of merges, */
			/* so a buffer of this size would allow it to skip using internal or in-place merges for anything */
			cache_size = sqrt(cache_size) + 1;
			cache = (Test *)malloc(cache_size * sizeof(array[0]));
			
			if (!cache) {
				/* 512 – chosen from careful testing as a good balance between fixed-size memory use and run time */
				if (cache_size > 512) {
					cache_size = 512;
					cache = (Test *)malloc(cache_size * sizeof(array[0]));
				}
				
				/* 0 – if the system simply cannot allocate any extra memory whatsoever, no memory works just fine */
				if (!cache) cache_size = 0;
			}
		}
	#endif
	
	
	/* merge sort the higher levels, which can be 8-15, 16-31, 32-63, 64-127, etc. */
	while (true) {
		
		/* if every A and B block will fit into the cache, use a special branch specifically for merging with the cache */
		/* (we use < rather than <= since the block size might be one more than iterator.length()) */
		if (WikiIterator_length(&iterator) < cache_size) {
			
			/* if four subarrays fit into the cache, it's faster to merge both pairs of subarrays into the cache, */
			/* then merge the two merged subarrays from the cache back into the original array */
			if ((WikiIterator_length(&iterator) + 1) * 4 <= cache_size && WikiIterator_length(&iterator) * 4 <= size) {
				
				WikiIterator_begin(&iterator);
				while (!WikiIterator_finished(&iterator)) {
					/* merge A1 and B1 into the cache */
					Range A1, B1, A2, B2, A3, B3;
					A1 = WikiIterator_nextRange(&iterator);
					B1 = WikiIterator_nextRange(&iterator);
					A2 = WikiIterator_nextRange(&iterator);
					B2 = WikiIterator_nextRange(&iterator);
					
					if (compare(array[B1.start], array[A1.end - 1])) {
						if (compare(array[B1.end - 1], array[A1.start])) {
							/* the two ranges are in reverse order, so copy them in reverse order into the cache */
							memcpy(&cache[Range_length(B1)], &array[A1.start], Range_length(A1) * sizeof(array[0]));
							memcpy(&cache[0], &array[B1.start], Range_length(B1) * sizeof(array[0]));
						} else {
							/* these two ranges weren't already in order, so merge them into the cache */
							MergeInto(array, A1, B1, compare, &cache[0]);
						}
					} else {
						/* if A1, B1, A2, and B2 are all in order, skip doing anything else */
						if (!compare(array[B2.start], array[A2.end - 1]) && !compare(array[A2.start], array[B1.end - 1])) continue;
						
						/* copy A1 and B1 into the cache in the same order */
						memcpy(&cache[0], &array[A1.start], Range_length(A1) * sizeof(array[0]));
						memcpy(&cache[Range_length(A1)], &array[B1.start], Range_length(B1) * sizeof(array[0]));
					}
					A1 = Range_new(A1.start, B1.end);
					
					/* merge A2 and B2 into the cache */
					if (compare(array[B2.start], array[A2.end - 1])) {
						if (compare(array[B2.end - 1], array[A2.start])) {
							/* the two ranges are in reverse order, so copy them in reverse order into the cache */
							memcpy(&cache[Range_length(A1) + Range_length(B2)], &array[A2.start], Range_length(A2) * sizeof(array[0]));
							memcpy(&cache[Range_length(A1)], &array[B2.start], Range_length(B2) * sizeof(array[0]));
						} else {
							/* these two ranges weren't already in order, so merge them into the cache */
							MergeInto(array, A2, B2, compare, &cache[Range_length(A1)]);
						}
					} else {
						/* copy A2 and B2 into the cache in the same order */
						memcpy(&cache[Range_length(A1)], &array[A2.start], Range_length(A2) * sizeof(array[0]));
						memcpy(&cache[Range_length(A1) + Range_length(A2)], &array[B2.start], Range_length(B2) * sizeof(array[0]));
					}
					A2 = Range_new(A2.start, B2.end);
					
					/* merge A1 and A2 from the cache into the array */
					A3 = Range_new(0, Range_length(A1));
					B3 = Range_new(Range_length(A1), Range_length(A1) + Range_length(A2));
					
					if (compare(cache[B3.start], cache[A3.end - 1])) {
						if (compare(cache[B3.end - 1], cache[A3.start])) {
							/* the two ranges are in reverse order, so copy them in reverse order into the array */
							memcpy(&array[A1.start + Range_length(A2)], &cache[A3.start], Range_length(A3) * sizeof(array[0]));
							memcpy(&array[A1.start], &cache[B3.start], Range_length(B3) * sizeof(array[0]));
						} else {
							/* these two ranges weren't already in order, so merge them back into the array */
							MergeInto(cache, A3, B3, compare, &array[A1.start]);
						}
					} else {
						/* copy A3 and B3 into the array in the same order */
						memcpy(&array[A1.start], &cache[A3.start], Range_length(A3) * sizeof(array[0]));
						memcpy(&array[A1.start + Range_length(A1)], &cache[B3.start], Range_length(B3) * sizeof(array[0]));
					}
				}
				
				/* we merged two levels at the same time, so we're done with this level already */
				/* (iterator.nextLevel() is called again at the bottom of this outer merge loop) */
				WikiIterator_nextLevel(&iterator);
				
			} else {
				/* for each A and B subarray, copy A into the cache and perform a standard merge */
				WikiIterator_begin(&iterator);
				while (!WikiIterator_finished(&iterator)) {
					Range A = WikiIterator_nextRange(&iterator);
					Range B = WikiIterator_nextRange(&iterator);
					
					if (compare(array[B.start], array[A.end - 1])) {
						if (compare(array[B.end - 1], array[A.start])) {
							/* the two ranges are in reverse order, so a simple rotation should fix it */
							Rotate(array, Range_length(A), Range_new(A.start, B.end), cache, cache_size);
						} else {
							/* these two ranges weren't already in order, so we'll need to merge them! */
							memcpy(&cache[0], &array[A.start], Range_length(A) * sizeof(array[0]));
							MergeExternal(array, A, B, compare, cache);
						}
					}
				}
			}
		} else {
			/* this is where the in-place merge logic starts!
			 
			 1. loop over the A and B subarrays within this level of the merge sort
			 if A and B need to be merged:
			 2. pull out two internal buffers each containing √A unique values
			 2a. adjust block_size and buffer_size if we couldn't find enough unique values
			 3. break A and B into blocks of size 'block_size'
			 4. "tag" each of the A blocks with values from the first internal buffer
			 5. roll the A blocks through the B blocks and drop/rotate them where they belong
			 6. merge each A block with any B values that follow, using the cache or the second internal buffer
			 7. sort the second internal buffer if it exists
			 8. redistribute the two internal buffers back into the array */
			
			bool extract_buffers = false;
			Range A, B;
			
			/* first handle any ranges that are in order or reverse order */
			WikiIterator_begin(&iterator);
			while (!WikiIterator_finished(&iterator)) {
				A = WikiIterator_nextRange(&iterator);
				B = WikiIterator_nextRange(&iterator);
				
				if (compare(array[A.end], array[A.end - 1])) {
					if (compare(array[B.end - 1], array[A.start])) {
						/* the two ranges are in reverse order, so a simple rotation should fix it */
						Rotate(array, Range_length(A), Range_new(A.start, B.end), cache, cache_size);
					} else {
						/* this range needs to be merged, so break out and create the two internal buffers */
						extract_buffers = true;
						break;
					}
				}
			}
			
			if (extract_buffers) {
				/* this code uses iterator, A, and B, but we still need the old values for later */
				WikiIterator old_iterator = iterator;
				Range old_A = A, old_B = B;
				
				size_t index, last, count, start, pull_index = 0;
				bool find_separately = false;
				
				/* the ideal size for each block is √A, which results in √A number of blocks too */
				size_t block_size = sqrt(WikiIterator_length(&iterator));
				size_t buffer_size = WikiIterator_length(&iterator)/block_size + 1;
				
				/* find two internal buffers of size 'buffer_size' each */
				size_t find = buffer_size + buffer_size;
				Range buffer1 = Range_new(0, 0);
				Range buffer2 = Range_new(0, 0);
				
				/* this is used for extracting and redistributing the internal buffers */
				/* 'count' values are pulled out from 'from' to 'to' (makes sense, right?) */
				/* after this level of merges is completed, the values are redistributed to within 'range' */
				struct { size_t from, to, count; Range range; } pull[2];
				pull[0].from = pull[0].to = pull[0].count = 0; pull[0].range = Range_new(0, 0);
				pull[1].from = pull[1].to = pull[1].count = 0; pull[1].range = Range_new(0, 0);
				
				if (block_size <= cache_size) {
					/* if every A block fits into the cache then we won't need the second internal buffer, */
					/* so we really only need to find 'buffer_size' unique values */
					find = buffer_size;
				} else if (find > WikiIterator_length(&iterator)) {
					/* we can't fit both buffers into the same A or B subarray, so find two buffers separately */
					find = buffer_size;
					find_separately = true;
				}
				
				/* we need to find either a single contiguous space containing 2√A unique values (which will be split up into two buffers of size √A each), */
				/* or we need to find one buffer of < 2√A unique values, and a second buffer of √A unique values, */
				/* OR if we couldn't find that many unique values, we need the largest possible buffer we can get */
				
				/* in the case where it couldn't find a single buffer of at least √A unique values, */
				/* all of the Merge steps must be replaced by a different merge algorithm (MergeInPlace) */
				
				WikiIterator_begin(&iterator);
				while (!WikiIterator_finished(&iterator)) {
					A = WikiIterator_nextRange(&iterator);
					B = WikiIterator_nextRange(&iterator);
					
					/* just store information about where the values will be pulled from and to, */
					/* as well as how many values there are, to create the two internal buffers */
					#define PULL(_to) \
						pull[pull_index].range = Range_new(A.start, B.end); \
						pull[pull_index].count = count; \
						pull[pull_index].from = index; \
						pull[pull_index].to = _to
					
					/* check A for the number of unique values we need to fill an internal buffer */
					/* these values will be pulled out to the start of A */
					for (last = A.start, count = 1; count < find; last = index, count++) {
						index = FindLastForward(array, array[last], Range_new(last + 1, A.end), compare, find - count);
						if (index == A.end) break;
					}
					index = last;
					
					if (count >= buffer_size) {
						/* keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer */
						PULL(A.start);
						pull_index = 1;
						
						if (count == buffer_size + buffer_size) {
							/* we were able to find a single contiguous section containing 2√A unique values, */
							/* so this section can be used to contain both of the internal buffers we'll need */
							buffer1 = Range_new(A.start, A.start + buffer_size);
							buffer2 = Range_new(A.start + buffer_size, A.start + count);
							break;
						} else if (find == buffer_size + buffer_size) {
							/* we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values, */
							/* so we still need to find a second separate buffer of at least √A unique values */
							buffer1 = Range_new(A.start, A.start + count);
							find = buffer_size;
						} else if (block_size <= cache_size) {
							/* we found the first and only internal buffer that we need, so we're done! */
							buffer1 = Range_new(A.start, A.start + count);
							break;
						} else if (find_separately) {
							/* found one buffer, but now find the other one */
							buffer1 = Range_new(A.start, A.start + count);
							find_separately = false;
						} else {
							/* we found a second buffer in an 'A' subarray containing √A unique values, so we're done! */
							buffer2 = Range_new(A.start, A.start + count);
							break;
						}
					} else if (pull_index == 0 && count > Range_length(buffer1)) {
						/* keep track of the largest buffer we were able to find */
						buffer1 = Range_new(A.start, A.start + count);
						PULL(A.start);
					}
					
					/* check B for the number of unique values we need to fill an internal buffer */
					/* these values will be pulled out to the end of B */
					for (last = B.end - 1, count = 1; count < find; last = index - 1, count++) {
						index = FindFirstBackward(array, array[last], Range_new(B.start, last), compare, find - count);
						if (index == B.start) break;
					}
					index = last;
					
					if (count >= buffer_size) {
						/* keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer */
						PULL(B.end);
						pull_index = 1;
						
						if (count == buffer_size + buffer_size) {
							/* we were able to find a single contiguous section containing 2√A unique values, */
							/* so this section can be used to contain both of the internal buffers we'll need */
							buffer1 = Range_new(B.end - count, B.end - buffer_size);
							buffer2 = Range_new(B.end - buffer_size, B.end);
							break;
						} else if (find == buffer_size + buffer_size) {
							/* we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values, */
							/* so we still need to find a second separate buffer of at least √A unique values */
							buffer1 = Range_new(B.end - count, B.end);
							find = buffer_size;
						} else if (block_size <= cache_size) {
							/* we found the first and only internal buffer that we need, so we're done! */
							buffer1 = Range_new(B.end - count, B.end);
							break;
						} else if (find_separately) {
							/* found one buffer, but now find the other one */
							buffer1 = Range_new(B.end - count, B.end);
							find_separately = false;
						} else {
							/* buffer2 will be pulled out from a 'B' subarray, so if the first buffer was pulled out from the corresponding 'A' subarray, */
							/* we need to adjust the end point for that A subarray so it knows to stop redistributing its values before reaching buffer2 */
							if (pull[0].range.start == A.start) pull[0].range.end -= pull[1].count;
							
							/* we found a second buffer in an 'B' subarray containing √A unique values, so we're done! */
							buffer2 = Range_new(B.end - count, B.end);
							break;
						}
					} else if (pull_index == 0 && count > Range_length(buffer1)) {
						/* keep track of the largest buffer we were able to find */
						buffer1 = Range_new(B.end - count, B.end);
						PULL(B.end);
					}
				}
				
				/* pull out the two ranges so we can use them as internal buffers */
				for (pull_index = 0; pull_index < 2; pull_index++) {
					Range range;
					size_t length = pull[pull_index].count;
					
					if (pull[pull_index].to < pull[pull_index].from) {
						/* we're pulling the values out to the left, which means the start of an A subarray */
						index = pull[pull_index].from;
						for (count = 1; count < length; count++) {
							index = FindFirstBackward(array, array[index - 1], Range_new(pull[pull_index].to, pull[pull_index].from - (count - 1)), compare, length - count);
							range = Range_new(index + 1, pull[pull_index].from + 1);
							Rotate(array, Range_length(range) - count, range, cache, cache_size);
							pull[pull_index].from = index + count;
						}
					} else if (pull[pull_index].to > pull[pull_index].from) {
						/* we're pulling values out to the right, which means the end of a B subarray */
						index = pull[pull_index].from + 1;
						for (count = 1; count < length; count++) {
							index = FindLastForward(array, array[index], Range_new(index, pull[pull_index].to), compare, length - count);
							range = Range_new(pull[pull_index].from, index - 1);
							Rotate(array, count, range, cache, cache_size);
							pull[pull_index].from = index - 1 - count;
						}
					}
				}
				
				/* adjust block_size and buffer_size based on the values we were able to pull out */
				buffer_size = Range_length(buffer1);
				block_size = WikiIterator_length(&iterator)/buffer_size + 1;
				
				/* the first buffer NEEDS to be large enough to tag each of the evenly sized A blocks, */
				/* so this was originally here to test the math for adjusting block_size above */
				/* assert((WikiIterator_length(&iterator) + 1)/block_size <= buffer_size); */
				
				/* restore the old values for the iterator and A and B */
				iterator = old_iterator;
				A = old_A; B = old_B;
				
				/* now that the two internal buffers have been created, it's time to merge each A+B combination at this level of the merge sort! */
				while (true) {
					
					/* remove any parts of A or B that are being used by the internal buffers */
					start = A.start;
					if (start == pull[0].range.start) {
						if (pull[0].from > pull[0].to) {
							A.start += pull[0].count;
							
							/* if the internal buffer takes up the entire A or B subarray, then there's nothing to merge */
							/* this only happens for very small subarrays, like √4 = 2, 2 * (2 internal buffers) = 4, */
							/* which also only happens when cache_size is small or 0 since it'd otherwise use MergeExternal */
							if (Range_length(A) == 0) continue;
						} else if (pull[0].from < pull[0].to) {
							B.end -= pull[0].count;
							if (Range_length(B) == 0) continue;
						}
					}
					if (start == pull[1].range.start) {
						if (pull[1].from > pull[1].to) {
							A.start += pull[1].count;
							if (Range_length(A) == 0) continue;
						} else if (pull[1].from < pull[1].to) {
							B.end -= pull[1].count;
							if (Range_length(B) == 0) continue;
						}
					}
					
					/* see if the data is already in order or reverse order */
					if (compare(array[A.end], array[A.end - 1])) {
						Range blockA, firstA, lastA, lastB, blockB;
						size_t indexA, findA;
						
						if (compare(array[B.end - 1], array[A.start])) {
							Rotate(array, Range_length(A), Range_new(A.start, B.end), cache, cache_size);
							continue;
						}
						
						/* break the remainder of A into blocks. firstA is the uneven-sized first A block */
						blockA = Range_new(A.start, A.end);
						firstA = Range_new(A.start, A.start + Range_length(blockA) % block_size);
						
						/* tag the A blocks by swapping the first values with the values in buffer1 */
						for (indexA = buffer1.start, index = firstA.end; index < blockA.end; indexA++, index += block_size) 
							Swap(array[indexA], array[index]);
						
						/* when we drop an A block behind we'll need to merge the previous A block with any B blocks that follow it, so track that information as well */
						lastA = firstA;
						lastB = Range_new(0, 0);
						blockB = Range_new(B.start, B.start + Min(block_size, Range_length(B)));
						blockA.start += Range_length(firstA);
						indexA = buffer1.start;
						
						/* if the first unevenly sized A block fits into the cache, copy it there for when we go to Merge it */
						/* otherwise, if the second buffer is available, block swap the contents into that */
						if (Range_length(lastA) <= cache_size)
							memcpy(&cache[0], &array[lastA.start], Range_length(lastA) * sizeof(array[0]));
						else if (Range_length(buffer2) > 0)
							BlockSwap(array, lastA.start, buffer2.start, Range_length(lastA));
						
						/* if there are no evenly-size A blocks, we only need to merge lastA with B and can skip this step */
						if (Range_length(blockA) > 0) {
							
							/* start rolling the A blocks through the B blocks! */
							while (true) {
								
								/* if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block, */
								/* then drop that minimum A block behind. or if there are no B blocks left then keep dropping the remaining A blocks. */
								if ((Range_length(lastB) > 0 && !compare(array[lastB.end - 1], array[indexA])) || Range_length(blockB) == 0) {
									
									/* figure out where to split the previous B block, and rotate it at the split */
									size_t B_split = BinaryFirst(array, array[indexA], lastB, compare);
									size_t B_remaining = lastB.end - B_split;
									
									/* swap the minimum A block to the beginning of the rolling A blocks */
									size_t minA = blockA.start;
									for (findA = minA + block_size; findA < blockA.end; findA += block_size)
										if (compare(array[findA], array[minA]))
											minA = findA;
									BlockSwap(array, blockA.start, minA, block_size);
									
									/* swap the first item of the previous A block back with its original value, which is stored in buffer1 */
									Swap(array[blockA.start], array[indexA]);
									indexA++;
									
									/*
									 locally merge the previous A block with the B values that follow it
									 if lastA fits into the external cache we'll use that (with MergeExternal),
									 or if the second internal buffer exists we'll use that (with MergeInternal),
									 or failing that we'll use a strictly in-place merge algorithm (MergeInPlace)
									 */
									if (Range_length(lastA) <= cache_size)
										MergeExternal(array, lastA, Range_new(lastA.end, B_split), compare, cache);
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
									
								} else if (Range_length(blockB) < block_size) {
									/* move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation */
									/* the cache is disabled here since it might contain the contents of the previous A block */
									Rotate(array, blockB.start - blockA.start, Range_new(blockA.start, blockB.end), cache, 0);
									
									lastB = Range_new(blockA.start, blockA.start + Range_length(blockB));
									blockA.start += Range_length(blockB);
									blockA.end += Range_length(blockB);
									blockB.end = blockB.start;
								} else {
									/* roll the leftmost A block to the end by swapping it with the next B block */
									BlockSwap(array, blockA.start, blockB.start, block_size);
									lastB = Range_new(blockA.start, blockA.start + block_size);
									
									blockA.start += block_size;
									blockA.end += block_size;
									blockB.start += block_size;
									
									if (blockB.end > B.end - block_size) blockB.end = B.end;
									else blockB.end += block_size;
								}
							}
						}
						
						/* merge the last A block with the remaining B values */
						if (Range_length(lastA) <= cache_size)
							MergeExternal(array, lastA, Range_new(lastA.end, B.end), compare, cache);
						else if (Range_length(buffer2) > 0)
							MergeInternal(array, lastA, Range_new(lastA.end, B.end), compare, buffer2);
						else
							MergeInPlace(array, lastA, Range_new(lastA.end, B.end), compare, cache, cache_size);
					}
					
					/* get the next set of ranges to merge */
					if (WikiIterator_finished(&iterator)) break;
					A = WikiIterator_nextRange(&iterator);
					B = WikiIterator_nextRange(&iterator);
				}
				
				/* when we're finished with this merge step we should have the one or two internal buffers left over, where the second buffer is all jumbled up */
				/* insertion sort the second buffer, then redistribute the buffers back into the array using the opposite process used for creating the buffer */
				
				/* while an unstable sort like quicksort could be applied here, in benchmarks it was consistently slightly slower than a simple insertion sort, */
				/* even for tens of millions of items. this may be because insertion sort is quite fast when the data is already somewhat sorted, like it is here */
				InsertionSort(array, buffer2, compare);
				
				for (pull_index = 0; pull_index < 2; pull_index++) {
					size_t amount, unique = pull[pull_index].count * 2;
					if (pull[pull_index].from > pull[pull_index].to) {
						/* the values were pulled out to the left, so redistribute them back to the right */
						Range buffer = Range_new(pull[pull_index].range.start, pull[pull_index].range.start + pull[pull_index].count);
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
		}
		
		/* double the size of each A and B subarray that will be merged in the next level */
		if (!WikiIterator_nextLevel(&iterator)) break;
	}
	
	#if DYNAMIC_CACHE
		if (cache) free(cache);
	#endif
	
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

/* the last 1/5 of the data is random */
size_t TestingAppend(size_t index, size_t total) {
	if (index > total - total/5) return rand() * 1.0/RAND_MAX * total;
	return index;
}

/* make sure the items within the given range are in a stable order */
/* if you want to test the correctness of any changes you make to the main WikiSort function,
 move this function to the top of the file and call it from within WikiSort after each step */
#if VERIFY
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
#endif

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
	#if !SLOW_COMPARISONS && VERIFY
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
			TestingMostlyEqual,
			TestingAppend
		};
	#endif
	
	/* initialize the random-number generator */
	/*srand(time(NULL));*/
	srand(10141985); /* in case you want the same random numbers */
	
	total = max_size;
	
#if !SLOW_COMPARISONS && VERIFY
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
			
			/* TestingRandom, TestingRandomFew, TestingMostlyDescending, TestingMostlyAscending, */
			/* TestingAscending, TestingDescending, TestingEqual, TestingJittered, TestingMostlyEqual, TestingAppend */
			item.value = TestingRandom(index, total);
			#if VERIFY
				item.index = index;
			#endif
			
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
		
		printf("[%zu]\n", total);
		
		if (time1 >= time2)
			printf("WikiSort: %f seconds, MergeSort: %f seconds (%f%% as fast)\n", time1, time2, time2/time1 * 100.0);
		else
			printf("WikiSort: %f seconds, MergeSort: %f seconds (%f%% faster)\n", time1, time2, time2/time1 * 100.0 - 100.0);
		
		#if PROFILE
			if (compares1 <= compares2)
				printf("WikiSort: %zu compares, MergeSort: %zu compares (%f%% as many)\n", compares1, compares2, compares1 * 100.0/compares2);
			else
				printf("WikiSort: %zu compares, MergeSort: %zu compares (%f%% more)\n", compares1, compares2, compares1 * 100.0/compares2 - 100.0);
		#endif
		
		#if VERIFY
			/* make sure the arrays are sorted correctly, and that the results were stable */
			printf("verifying... ");
			fflush(stdout);
			
			WikiVerify(array1, Range_new(0, total), compare, "testing the final array");
			for (index = 0; index < total; index++)
				assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
			
			printf("correct!\n");
		#endif
	}
	
	total_time = Seconds() - total_time;
	printf("tests completed in %f seconds\n", total_time);
	if (total_time1 >= total_time2)
		printf("WikiSort: %f seconds, MergeSort: %f seconds (%f%% as fast)\n", total_time1, total_time2, total_time2/total_time1 * 100.0);
	else
		printf("WikiSort: %f seconds, MergeSort: %f seconds (%f%% faster)\n", total_time1, total_time2, total_time2/total_time1 * 100.0 - 100.0);
	
	#if PROFILE
		if (total_compares1 <= total_compares2)
			printf("WikiSort: %zu compares, MergeSort: %zu compares (%f%% as many)\n", total_compares1, total_compares2, total_compares1 * 100.0/total_compares2);
		else
			printf("WikiSort: %zu compares, MergeSort: %zu compares (%f%% more)\n", total_compares1, total_compares2, total_compares1 * 100.0/total_compares2 - 100.0);
	#endif
	
	free(array1); free(array2);
	return 0;
}
