/***********************************************************
 WikiSort: a public domain implementation of "Block Sort"
 https://github.com/BonzaiThePenguin/WikiSort

 to run:
 clang++ -o WikiSort.x WikiSort.cpp -O3
 (or replace 'clang++' with 'g++')
 ./WikiSort.x
***********************************************************/

#include <iostream>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cassert>
#include <cstring>
#include <ctime>

// record the number of comparisons and assignments
// note that this reduces WikiSort's performance when enabled
#define PROFILE false

// verify that WikiSort is actually correct
// (this also reduces performance slightly)
#define VERIFY false

// simulate comparisons that have a bit more overhead than just an inlined (int < int)
// (so we can tell whether reducing the number of comparisons was worth the added complexity)
#define SLOW_COMPARISONS false

// if true, test against std::__inplace_stable_sort() rather than std::stable_sort()
#define TEST_INPLACE false

// whether to give WikiSort a full-size cache, to see how it performs when given more memory
#define DYNAMIC_CACHE false


double Seconds() { return clock() * 1.0/CLOCKS_PER_SEC; }

#if PROFILE
	// global for testing how many comparisons are performed for each sorting algorithm
	long comparisons, assignments;
#endif

// structure to represent ranges within the array
class Range {
public:
	size_t start;
	size_t end;

	Range() {}
	Range(size_t start, size_t end) : start(start), end(end) {}
	size_t length() const { return end - start; }
};

// toolbox functions used by the sorter

// 63 -> 32, 64 -> 64, etc.
// this comes from Hacker's Delight
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

// find the index of the first value within the range that is equal to array[index]
template <typename T, typename Comparison>
size_t BinaryFirst(const T array[], const T & value, const Range & range, const Comparison compare) {
	return std::lower_bound(&array[range.start], &array[range.end], value, compare) - &array[0];
}

// find the index of the last value within the range that is equal to array[index], plus 1
template <typename T, typename Comparison>
size_t BinaryLast(const T array[], const T & value, const Range & range, const Comparison compare) {
	return std::upper_bound(&array[range.start], &array[range.end], value, compare) - &array[0];
}

// combine a linear search with a binary search to reduce the number of comparisons in situations
// where have some idea as to how many unique values there are and where the next value might be
template <typename T, typename Comparison>
size_t FindFirstForward(const T array[], const T & value, const Range & range, const Comparison compare, const size_t unique) {
	if (range.length() == 0) return range.start;
	size_t index, skip = std::max(range.length()/unique, (size_t)1);

	for (index = range.start + skip; compare(array[index - 1], value); index += skip)
		if (index >= range.end - skip)
			return BinaryFirst(array, value, Range(index, range.end), compare);

	return BinaryFirst(array, value, Range(index - skip, index), compare);
}

template <typename T, typename Comparison>
size_t FindLastForward(const T array[], const T & value, const Range & range, const Comparison compare, const size_t unique) {
	if (range.length() == 0) return range.start;
	size_t index, skip = std::max(range.length()/unique, (size_t)1);

	for (index = range.start + skip; !compare(value, array[index - 1]); index += skip)
		if (index >= range.end - skip)
			return BinaryLast(array, value, Range(index, range.end), compare);

	return BinaryLast(array, value, Range(index - skip, index), compare);
}

template <typename T, typename Comparison>
size_t FindFirstBackward(const T array[], const T & value, const Range & range, const Comparison compare, const size_t unique) {
	if (range.length() == 0) return range.start;
	size_t index, skip = std::max(range.length()/unique, (size_t)1);

	for (index = range.end - skip; index > range.start && !compare(array[index - 1], value); index -= skip)
		if (index < range.start + skip)
			return BinaryFirst(array, value, Range(range.start, index), compare);

	return BinaryFirst(array, value, Range(index, index + skip), compare);
}

template <typename T, typename Comparison>
size_t FindLastBackward(const T array[], const T & value, const Range & range, const Comparison compare, const size_t unique) {
	if (range.length() == 0) return range.start;
	size_t index, skip = std::max(range.length()/unique, (size_t)1);

	for (index = range.end - skip; index > range.start && compare(value, array[index - 1]); index -= skip)
		if (index < range.start + skip)
			return BinaryLast(array, value, Range(range.start, index), compare);

	return BinaryLast(array, value, Range(index, index + skip), compare);
}

// n^2 sorting algorithm used to sort tiny chunks of the full array
template <typename T, typename Comparison>
void InsertionSort(T array[], const Range & range, const Comparison compare) {
	for (size_t j, i = range.start + 1; i < range.end; i++) {
		const T temp = array[i];
		for (j = i; j > range.start && compare(temp, array[j - 1]); j--)
			array[j] = array[j - 1];
		array[j] = temp;
	}
}

// reverse a range of values within the array
template <typename T>
void Reverse(T array[], const Range & range) {
	std::reverse(&array[range.start], &array[range.end]);
}

// swap a series of values in the array
template <typename T>
void BlockSwap(T array[], const size_t start1, const size_t start2, const size_t block_size) {
	std::swap_ranges(&array[start1], &array[start1 + block_size], &array[start2]);
}

// rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1)
// this assumes that 0 <= amount <= range.length()
template <typename T>
void Rotate(T array[], size_t amount, Range range) {
	std::rotate(&array[range.start], &array[range.start + amount], &array[range.end]);
}

namespace Wiki {
	// merge two ranges from one array and save the results into a different array
	template <typename T, typename Comparison>
	void MergeInto(T from[], const Range & A, const Range & B, const Comparison compare, T into[]) {
		T *A_index = &from[A.start], *B_index = &from[B.start];
		T *A_last = &from[A.end], *B_last = &from[B.end];
		T *insert_index = &into[0];

		while (true) {
			if (!compare(*B_index, *A_index)) {
				*insert_index = *A_index;
				A_index++;
				insert_index++;
				if (A_index == A_last) {
					// copy the remainder of B into the final array
					std::copy(B_index, B_last, insert_index);
					break;
				}
			} else {
				*insert_index = *B_index;
				B_index++;
				insert_index++;
				if (B_index == B_last) {
					// copy the remainder of A into the final array
					std::copy(A_index, A_last, insert_index);
					break;
				}
			}
		}
	}

	// merge operation using an external buffer
	template <typename T, typename Comparison>
	void MergeExternal(T array[], const Range & A, const Range & B, const Comparison compare, T cache[]) {
		// A fits into the cache, so use that instead of the internal buffer
		T *A_index = &cache[0], *B_index = &array[B.start];
		T *A_last = &cache[A.length()], *B_last = &array[B.end];
		T *insert_index = &array[A.start];

		if (B.length() > 0 && A.length() > 0) {
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

		// copy the remainder of A into the final array
		std::copy(A_index, A_last, insert_index);
	}

	// merge operation using an internal buffer
	template <typename T, typename Comparison>
	void MergeInternal(T array[], const Range & A, const Range & B, const Comparison compare, const Range & buffer) {
		// whenever we find a value to add to the final array, swap it with the value that's already in that spot
		// when this algorithm is finished, 'buffer' will contain its original contents, but in a different order
		T *A_index = &array[buffer.start], *B_index = &array[B.start];
		T *A_last = &array[buffer.start + A.length()], *B_last = &array[B.end];
		T *insert_index = &array[A.start];

		if (B.length() > 0 && A.length() > 0) {
			while (true) {
				if (!compare(*B_index, *A_index)) {
					std::iter_swap(insert_index, A_index);
					A_index++;
					insert_index++;
					if (A_index == A_last) break;
				} else {
					std::iter_swap(insert_index, B_index);
					B_index++;
					insert_index++;
					if (B_index == B_last) break;
				}
			}
		}

		// BlockSwap
		std::swap_ranges(A_index, A_last, insert_index);
	}

	// merge operation without a buffer
	template <typename T, typename Comparison>
	void MergeInPlace(T array[], Range A, Range B, const Comparison compare) {
		if (A.length() == 0 || B.length() == 0) return;

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
			// find the first place in B where the first item in A needs to be inserted
			size_t mid = BinaryFirst(array, array[A.start], B, compare);

			// rotate A into place
			size_t amount = mid - A.end;
			Rotate(array, A.length(), Range(A.start, mid));
			if (B.end == mid) break;

			// calculate the new A and B ranges
			B.start = mid;
			A = Range(A.start + amount, B.start);
			A.start = BinaryLast(array, array[A.start], A, compare);
			if (A.length() == 0) break;
		}
	}

	// calculate how to scale the index value to the range within the array
	// the bottom-up merge sort only operates on values that are powers of two,
	// so scale down to that power of two, then use a fraction to scale back again
	class Iterator {
		size_t size, power_of_two;
		size_t decimal, numerator, denominator;
		size_t decimal_step, numerator_step;

	public:
		Iterator(size_t size2, size_t min_level) {
			size = size2;
			power_of_two = FloorPowerOfTwo(size);
			denominator = power_of_two/min_level;
			numerator_step = size % denominator;
			decimal_step = size/denominator;
			begin();
		}

		void begin() {
			numerator = decimal = 0;
		}

		Range nextRange() {
			size_t start = decimal;

			decimal += decimal_step;
			numerator += numerator_step;
			if (numerator >= denominator) {
				numerator -= denominator;
				decimal++;
			}

			return Range(start, decimal);
		}

		bool finished() {
			return (decimal >= size);
		}

		bool nextLevel() {
			decimal_step += decimal_step;
			numerator_step += numerator_step;
			if (numerator_step >= denominator) {
				numerator_step -= denominator;
				decimal_step++;
			}

			return (decimal_step < size);
		}

		size_t length() {
			return decimal_step;
		}
	};

#if DYNAMIC_CACHE
	// use a class so the memory for the cache is freed when the object goes out of scope,
	// regardless of whether exceptions were thrown (only needed in the C++ version)
	template <typename T>
	class Cache {
	public:
		T *cache;
		size_t cache_size;

		~Cache() {
			if (cache) delete[] cache;
		}

		Cache(size_t size) {
			// good choices for the cache size are:
			// (size + 1)/2 – turns into a full-speed standard merge sort since everything fits into the cache
			cache_size = (size + 1)/2;
			cache = new (std::nothrow) T[cache_size];
			if (cache) return;

			// sqrt((size + 1)/2) + 1 – this will be the size of the A blocks at the largest level of merges,
			// so a buffer of this size would allow it to skip using internal or in-place merges for anything
			cache_size = sqrt(cache_size) + 1;
			cache = new (std::nothrow) T[cache_size];
			if (cache) return;

			// 512 – chosen from careful testing as a good balance between fixed-size memory use and run time
			if (cache_size > 512) {
				cache_size = 512;
				cache = new (std::nothrow) T[cache_size];
				if (cache) return;
			}

			// 0 – if the system simply cannot allocate any extra memory whatsoever, no memory works just fine
			cache_size = 0;
		}
	};
#endif

	// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
	template <typename Iterator, typename Comparison>
	void Sort(Iterator first, Iterator last, const Comparison compare) {
		// map first and last to a C-style array, so we don't have to change the rest of the code
		// (bit of a nasty hack, but it's good enough for now...)
		typedef typename std::iterator_traits<Iterator>::value_type T;
		const size_t size = last - first;
		T *array = &first[0];

		// if the array is of size 0, 1, 2, or 3, just sort them like so:
		if (size < 4) {
			if (size == 3) {
				// hard-coded insertion sort
				if (compare(array[1], array[0])) {
                    std::iter_swap(array + 0, array + 1);
				}
				if (compare(array[2], array[1])) {
					std::iter_swap(array + 1, array + 2);
					if (compare(array[1], array[0])) {
                        std::iter_swap(array + 0, array + 1);
					}
				}
			} else if (size == 2) {
				// swap the items if they're out of order
				if (compare(array[1], array[0])) {
                    std::iter_swap(array + 0, array + 1);
				}
			}

			return;
		}

		// sort groups of 4-8 items at a time using an unstable sorting network,
		// but keep track of the original item orders to force it to be stable
		// http://pages.ripco.net/~jgamble/nw.html
		Wiki::Iterator iterator (size, 4);
		while (!iterator.finished()) {
			uint8_t order[] = { 0, 1, 2, 3, 4, 5, 6, 7 };
			Range range = iterator.nextRange();

			#define SWAP(x, y) \
				if (compare(array[range.start + y], array[range.start + x]) || \
					(order[x] > order[y] && !compare(array[range.start + x], array[range.start + y]))) { \
					std::iter_swap(array + range.start + x, array + range.start + y); \
					std::iter_swap(order + x, order + y); }

			if (range.length() == 8) {
				SWAP(0, 1); SWAP(2, 3); SWAP(4, 5); SWAP(6, 7);
				SWAP(0, 2); SWAP(1, 3); SWAP(4, 6); SWAP(5, 7);
				SWAP(1, 2); SWAP(5, 6); SWAP(0, 4); SWAP(3, 7);
				SWAP(1, 5); SWAP(2, 6);
				SWAP(1, 4); SWAP(3, 6);
				SWAP(2, 4); SWAP(3, 5);
				SWAP(3, 4);

			} else if (range.length() == 7) {
				SWAP(1, 2); SWAP(3, 4); SWAP(5, 6);
				SWAP(0, 2); SWAP(3, 5); SWAP(4, 6);
				SWAP(0, 1); SWAP(4, 5); SWAP(2, 6);
				SWAP(0, 4); SWAP(1, 5);
				SWAP(0, 3); SWAP(2, 5);
				SWAP(1, 3); SWAP(2, 4);
				SWAP(2, 3);

			} else if (range.length() == 6) {
				SWAP(1, 2); SWAP(4, 5);
				SWAP(0, 2); SWAP(3, 5);
				SWAP(0, 1); SWAP(3, 4); SWAP(2, 5);
				SWAP(0, 3); SWAP(1, 4);
				SWAP(2, 4); SWAP(1, 3);
				SWAP(2, 3);

			} else if (range.length() == 5) {
				SWAP(0, 1); SWAP(3, 4);
				SWAP(2, 4);
				SWAP(2, 3); SWAP(1, 4);
				SWAP(0, 3);
				SWAP(0, 2); SWAP(1, 3);
				SWAP(1, 2);

			} else if (range.length() == 4) {
				SWAP(0, 1); SWAP(2, 3);
				SWAP(0, 2); SWAP(1, 3);
				SWAP(1, 2);
			}
		}
		if (size < 8) return;

		// use a small cache to speed up some of the operations
		#if DYNAMIC_CACHE
			Cache<T> cache_obj (size);
			T *cache = cache_obj.cache;
			const size_t cache_size = cache_obj.cache_size;
		#else
			// since the cache size is fixed, it's still O(1) memory!
			// just keep in mind that making it too small ruins the point (nothing will fit into it),
			// and making it too large also ruins the point (so much for "low memory"!)
			// removing the cache entirely still gives 75% of the performance of a standard merge
			const size_t cache_size = 512;
			T cache[cache_size];
		#endif

		// then merge sort the higher levels, which can be 8-15, 16-31, 32-63, 64-127, etc.
		while (true) {
			// if every A and B block will fit into the cache, use a special branch specifically for merging with the cache
			// (we use < rather than <= since the block size might be one more than iterator.length())
			if (iterator.length() < cache_size) {

				// if four subarrays fit into the cache, it's faster to merge both pairs of subarrays into the cache,
				// then merge the two merged subarrays from the cache back into the original array
				if ((iterator.length() + 1) * 4 <= cache_size && iterator.length() * 4 <= size) {
					iterator.begin();
					while (!iterator.finished()) {
						// merge A1 and B1 into the cache
						Range A1 = iterator.nextRange();
						Range B1 = iterator.nextRange();
						Range A2 = iterator.nextRange();
						Range B2 = iterator.nextRange();

						if (compare(array[B1.end - 1], array[A1.start])) {
							// the two ranges are in reverse order, so copy them in reverse order into the cache
							std::copy(&array[A1.start], &array[A1.end], &cache[B1.length()]);
							std::copy(&array[B1.start], &array[B1.end], &cache[0]);
						} else if (compare(array[B1.start], array[A1.end - 1])) {
							// these two ranges weren't already in order, so merge them into the cache
							MergeInto(array, A1, B1, compare, &cache[0]);
						} else {
							// if A1, B1, A2, and B2 are all in order, skip doing anything else
							if (!compare(array[B2.start], array[A2.end - 1]) && !compare(array[A2.start], array[B1.end - 1])) continue;

							// copy A1 and B1 into the cache in the same order
							std::copy(&array[A1.start], &array[A1.end], &cache[0]);
							std::copy(&array[B1.start], &array[B1.end], &cache[A1.length()]);
						}
						A1 = Range(A1.start, B1.end);

						// merge A2 and B2 into the cache
						if (compare(array[B2.end - 1], array[A2.start])) {
							// the two ranges are in reverse order, so copy them in reverse order into the cache
							std::copy(&array[A2.start], &array[A2.end], &cache[A1.length() + B2.length()]);
							std::copy(&array[B2.start], &array[B2.end], &cache[A1.length()]);
						} else if (compare(array[B2.start], array[A2.end - 1])) {
							// these two ranges weren't already in order, so merge them into the cache
							MergeInto(array, A2, B2, compare, &cache[A1.length()]);
						} else {
							// copy A2 and B2 into the cache in the same order
							std::copy(&array[A2.start], &array[A2.end], &cache[A1.length()]);
							std::copy(&array[B2.start], &array[B2.end], &cache[A1.length() + A2.length()]);
						}
						A2 = Range(A2.start, B2.end);

						// merge A1 and A2 from the cache into the array
						Range A3 = Range(0, A1.length());
						Range B3 = Range(A1.length(), A1.length() + A2.length());

						if (compare(cache[B3.end - 1], cache[A3.start])) {
							// the two ranges are in reverse order, so copy them in reverse order into the array
							std::copy(&cache[A3.start], &cache[A3.end], &array[A1.start + A2.length()]);
							std::copy(&cache[B3.start], &cache[B3.end], &array[A1.start]);
						} else if (compare(cache[B3.start], cache[A3.end - 1])) {
							// these two ranges weren't already in order, so merge them back into the array
							MergeInto(cache, A3, B3, compare, &array[A1.start]);
						} else {
							// copy A3 and B3 into the array in the same order
							std::copy(&cache[A3.start], &cache[A3.end], &array[A1.start]);
							std::copy(&cache[B3.start], &cache[B3.end], &array[A1.start + A1.length()]);
						}
					}

					// we merged two levels at the same time, so we're done with this level already
					// (iterator.nextLevel() is called again at the bottom of this outer merge loop)
					iterator.nextLevel();

				} else {
					iterator.begin();
					while (!iterator.finished()) {
						Range A = iterator.nextRange();
						Range B = iterator.nextRange();

						if (compare(array[B.end - 1], array[A.start])) {
							// the two ranges are in reverse order, so a simple rotation should fix it
							Rotate(array, A.length(), Range(A.start, B.end));
						} else if (compare(array[B.start], array[A.end - 1])) {
							// these two ranges weren't already in order, so we'll need to merge them!
							std::copy(&array[A.start], &array[A.end], &cache[0]);
							MergeExternal(array, A, B, compare, cache);
						}
					}
				}
			} else {
				// this is where the in-place merge logic starts!
				// 1. pull out two internal buffers each containing √A unique values
				//     1a. adjust block_size and buffer_size if we couldn't find enough unique values
				// 2. loop over the A and B subarrays within this level of the merge sort
				//     3. break A and B into blocks of size 'block_size'
				//     4. "tag" each of the A blocks with values from the first internal buffer
				//     5. roll the A blocks through the B blocks and drop/rotate them where they belong
				//     6. merge each A block with any B values that follow, using the cache or the second internal buffer
				// 7. sort the second internal buffer if it exists
				// 8. redistribute the two internal buffers back into the array

				size_t block_size = sqrt(iterator.length());
				size_t buffer_size = iterator.length()/block_size + 1;

				// as an optimization, we really only need to pull out the internal buffers once for each level of merges
				// after that we can reuse the same buffers over and over, then redistribute it when we're finished with this level
				Range buffer1 = Range(0, 0), buffer2 = Range(0, 0);
				size_t index, last, count, pull_index = 0;
				struct { size_t from, to, count; Range range; } pull[2];
				pull[0].from = pull[0].to = pull[0].count = 0; pull[0].range = Range(0, 0);
				pull[1].from = pull[1].to = pull[1].count = 0; pull[1].range = Range(0, 0);

				// find two internal buffers of size 'buffer_size' each
				// let's try finding both buffers at the same time from a single A or B subarray
				size_t find = buffer_size + buffer_size;
				bool find_separately = false;

				if (block_size <= cache_size) {
					// if every A block fits into the cache then we won't need the second internal buffer,
					// so we really only need to find 'buffer_size' unique values
					find = buffer_size;
				} else if (find > iterator.length()) {
					// we can't fit both buffers into the same A or B subarray, so find two buffers separately
					find = buffer_size;
					find_separately = true;
				}

				// we need to find either a single contiguous space containing 2√A unique values (which will be split up into two buffers of size √A each),
				// or we need to find one buffer of < 2√A unique values, and a second buffer of √A unique values,
				// OR if we couldn't find that many unique values, we need the largest possible buffer we can get

				// in the case where it couldn't find a single buffer of at least √A unique values,
				// all of the Merge steps must be replaced by a different merge algorithm (MergeInPlace)

				iterator.begin();
				while (!iterator.finished()) {
					Range A = iterator.nextRange();
					Range B = iterator.nextRange();

					// just store information about where the values will be pulled from and to,
					// as well as how many values there are, to create the two internal buffers
					#define PULL(_to) \
						pull[pull_index].range = Range(A.start, B.end); \
						pull[pull_index].count = count; \
						pull[pull_index].from = index; \
						pull[pull_index].to = _to

					// check A for the number of unique values we need to fill an internal buffer
					// these values will be pulled out to the start of A
					for (last = A.start, count = 1; count < find; last = index, count++) {
						index = FindLastForward(array, array[last], Range(last + 1, A.end), compare, find - count);
						if (index == A.end) break;
						assert(index < A.end);
					}
					index = last;

					if (count >= buffer_size) {
						// keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer
						PULL(A.start);
						pull_index = 1;

						if (count == buffer_size + buffer_size) {
							// we were able to find a single contiguous section containing 2√A unique values,
							// so this section can be used to contain both of the internal buffers we'll need
							buffer1 = Range(A.start, A.start + buffer_size);
							buffer2 = Range(A.start + buffer_size, A.start + count);
							break;
						} else if (find == buffer_size + buffer_size) {
							// we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values,
							// so we still need to find a second separate buffer of at least √A unique values
							buffer1 = Range(A.start, A.start + count);
							find = buffer_size;
						} else if (block_size <= cache_size) {
							// we found the first and only internal buffer that we need, so we're done!
							buffer1 = Range(A.start, A.start + count);
							break;
						} else if (find_separately) {
							// found one buffer, but now find the other one
							buffer1 = Range(A.start, A.start + count);
							find_separately = false;
						} else {
							// we found a second buffer in an 'A' subarray containing √A unique values, so we're done!
							buffer2 = Range(A.start, A.start + count);
							break;
						}
					} else if (pull_index == 0 && count > buffer1.length()) {
						// keep track of the largest buffer we were able to find
						buffer1 = Range(A.start, A.start + count);
						PULL(A.start);
					}

					// check B for the number of unique values we need to fill an internal buffer
					// these values will be pulled out to the end of B
					for (last = B.end - 1, count = 1; count < find; last = index - 1, count++) {
						index = FindFirstBackward(array, array[last], Range(B.start, last), compare, find - count);
						if (index == B.start) break;
						assert(index > B.start);
					}
					index = last;

					if (count >= buffer_size) {
						// keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer
						PULL(B.end);
						pull_index = 1;

						if (count == buffer_size + buffer_size) {
							// we were able to find a single contiguous section containing 2√A unique values,
							// so this section can be used to contain both of the internal buffers we'll need
							buffer1 = Range(B.end - count, B.end - buffer_size);
							buffer2 = Range(B.end - buffer_size, B.end);
							break;
						} else if (find == buffer_size + buffer_size) {
							// we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values,
							// so we still need to find a second separate buffer of at least √A unique values
							buffer1 = Range(B.end - count, B.end);
							find = buffer_size;
						} else if (block_size <= cache_size) {
							// we found the first and only internal buffer that we need, so we're done!
							buffer1 = Range(B.end - count, B.end);
							break;
						} else if (find_separately) {
							// found one buffer, but now find the other one
							buffer1 = Range(B.end - count, B.end);
							find_separately = false;
						} else {
							// buffer2 will be pulled out from a 'B' subarray, so if the first buffer was pulled out from the corresponding 'A' subarray,
							// we need to adjust the end point for that A subarray so it knows to stop redistributing its values before reaching buffer2
							if (pull[0].range.start == A.start) pull[0].range.end -= pull[1].count;

							// we found a second buffer in a 'B' subarray containing √A unique values, so we're done!
							buffer2 = Range(B.end - count, B.end);
							break;
						}
					} else if (pull_index == 0 && count > buffer1.length()) {
						// keep track of the largest buffer we were able to find
						buffer1 = Range(B.end - count, B.end);
						PULL(B.end);
					}
				}

				// pull out the two ranges so we can use them as internal buffers
				for (pull_index = 0; pull_index < 2; pull_index++) {
					size_t length = pull[pull_index].count;

					if (pull[pull_index].to < pull[pull_index].from) {
						// we're pulling the values out to the left, which means the start of an A subarray
						index = pull[pull_index].from;
						for (count = 1; count < length; count++) {
							index = FindFirstBackward(array, array[index - 1], Range(pull[pull_index].to, pull[pull_index].from - (count - 1)), compare, length - count);
							Range range = Range(index + 1, pull[pull_index].from + 1);
							Rotate(array, range.length() - count, range);
							pull[pull_index].from = index + count;
						}
					} else if (pull[pull_index].to > pull[pull_index].from) {
						// we're pulling values out to the right, which means the end of a B subarray
						index = pull[pull_index].from + 1;
						for (count = 1; count < length; count++) {
							index = FindLastForward(array, array[index], Range(index, pull[pull_index].to), compare, length - count);
							Range range = Range(pull[pull_index].from, index - 1);
							Rotate(array, count, range);
							pull[pull_index].from = index - 1 - count;
						}
					}
				}

				// adjust block_size and buffer_size based on the values we were able to pull out
				buffer_size = buffer1.length();
				block_size = iterator.length()/buffer_size + 1;

				// the first buffer NEEDS to be large enough to tag each of the evenly sized A blocks,
				// so this was originally here to test the math for adjusting block_size above
				//assert((iterator.length() + 1)/block_size <= buffer_size);

				// now that the two internal buffers have been created, it's time to merge each A+B combination at this level of the merge sort!
				iterator.begin();
				while (!iterator.finished()) {
					Range A = iterator.nextRange();
					Range B = iterator.nextRange();

					// remove any parts of A or B that are being used by the internal buffers
					size_t start = A.start;
					if (start == pull[0].range.start) {
						if (pull[0].from > pull[0].to) {
							A.start += pull[0].count;

							// if the internal buffer takes up the entire A or B subarray, then there's nothing to merge
							// this only happens for very small subarrays, like √4 = 2, 2 * (2 internal buffers) = 4,
							// which also only happens when cache_size is small or 0 since it'd otherwise use MergeExternal
							if (A.length() == 0) continue;
						} else if (pull[0].from < pull[0].to) {
							B.end -= pull[0].count;
							if (B.length() == 0) continue;
						}
					}
					if (start == pull[1].range.start) {
						if (pull[1].from > pull[1].to) {
							A.start += pull[1].count;
							if (A.length() == 0) continue;
						} else if (pull[1].from < pull[1].to) {
							B.end -= pull[1].count;
							if (B.length() == 0) continue;
						}
					}

					if (compare(array[B.end - 1], array[A.start])) {
						// the two ranges are in reverse order, so a simple rotation should fix it
						Rotate(array, A.length(), Range(A.start, B.end));
					} else if (compare(array[A.end], array[A.end - 1])) {
						// these two ranges weren't already in order, so we'll need to merge them!

						// break the remainder of A into blocks. firstA is the uneven-sized first A block
						Range blockA = Range(A.start, A.end);
						Range firstA = Range(A.start, A.start + blockA.length() % block_size);

						// swap the first value of each A block with the values in buffer1
						for (size_t indexA = buffer1.start, index = firstA.end; index < blockA.end; indexA++, index += block_size)
							std::iter_swap(array + indexA, array + index);

						// start rolling the A blocks through the B blocks!
						// when we leave an A block behind we'll need to merge the previous A block with any B blocks that follow it, so track that information as well
						Range lastA = firstA;
						Range lastB = Range(0, 0);
						Range blockB = Range(B.start, B.start + std::min(block_size, B.length()));
						blockA.start += firstA.length();
						size_t indexA = buffer1.start;

						// if the first unevenly sized A block fits into the cache, copy it there for when we go to Merge it
						// otherwise, if the second buffer is available, block swap the contents into that
						if (lastA.length() <= cache_size)
							std::copy(&array[lastA.start], &array[lastA.end], &cache[0]);
						else if (buffer2.length() > 0)
							BlockSwap(array, lastA.start, buffer2.start, lastA.length());

						if (blockA.length() > 0) {
							while (true) {
								// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block,
								// then drop that minimum A block behind. or if there are no B blocks left then keep dropping the remaining A blocks.
								if ((lastB.length() > 0 && !compare(array[lastB.end - 1], array[indexA])) || blockB.length() == 0) {
									// figure out where to split the previous B block, and rotate it at the split
									size_t B_split = BinaryFirst(array, array[indexA], lastB, compare);
									size_t B_remaining = lastB.end - B_split;

									// swap the minimum A block to the beginning of the rolling A blocks
									size_t minA = blockA.start;
									for (size_t findA = minA + block_size; findA < blockA.end; findA += block_size)
										if (compare(array[findA], array[minA]))
											minA = findA;
									BlockSwap(array, blockA.start, minA, block_size);

									// swap the first item of the previous A block back with its original value, which is stored in buffer1
									std::iter_swap(array + blockA.start, array + indexA);
									indexA++;

									// locally merge the previous A block with the B values that follow it
									// if lastA fits into the external cache we'll use that (with MergeExternal),
									// or if the second internal buffer exists we'll use that (with MergeInternal),
									// or failing that we'll use a strictly in-place merge algorithm (MergeInPlace)
									if (lastA.length() <= cache_size)
										MergeExternal(array, lastA, Range(lastA.end, B_split), compare, cache);
									else if (buffer2.length() > 0)
										MergeInternal(array, lastA, Range(lastA.end, B_split), compare, buffer2);
									else
										MergeInPlace(array, lastA, Range(lastA.end, B_split), compare);

									if (buffer2.length() > 0 || block_size <= cache_size) {
										// copy the previous A block into the cache or buffer2, since that's where we need it to be when we go to merge it anyway
										if (block_size <= cache_size)
											std::copy(&array[blockA.start], &array[blockA.start + block_size], cache);
										else
											BlockSwap(array, blockA.start, buffer2.start, block_size);

										// this is equivalent to rotating, but faster
										// the area normally taken up by the A block is either the contents of buffer2, or data we don't need anymore since we memcopied it
										// either way we don't need to retain the order of those items, so instead of rotating we can just block swap B to where it belongs
										BlockSwap(array, B_split, blockA.start + block_size - B_remaining, B_remaining);
									} else {
										// we are unable to use the 'buffer2' trick to speed up the rotation operation since buffer2 doesn't exist, so perform a normal rotation
										Rotate(array, blockA.start - B_split, Range(B_split, blockA.start + block_size));
									}

									// update the range for the remaining A blocks, and the range remaining from the B block after it was split
									lastA = Range(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
									lastB = Range(lastA.end, lastA.end + B_remaining);

									// if there are no more A blocks remaining, this step is finished!
									blockA.start += block_size;
									if (blockA.length() == 0)
										break;

								} else if (blockB.length() < block_size) {
									// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
									Rotate(array, blockB.start - blockA.start, Range(blockA.start, blockB.end));

									lastB = Range(blockA.start, blockA.start + blockB.length());
									blockA.start += blockB.length();
									blockA.end += blockB.length();
									blockB.end = blockB.start;
								} else {
									// roll the leftmost A block to the end by swapping it with the next B block
									BlockSwap(array, blockA.start, blockB.start, block_size);
									lastB = Range(blockA.start, blockA.start + block_size);

									blockA.start += block_size;
									blockA.end += block_size;
									blockB.start += block_size;

									if (blockB.end > B.end - block_size) blockB.end = B.end;
									else blockB.end += block_size;
								}
							}
						}

						// merge the last A block with the remaining B values
						if (lastA.length() <= cache_size)
							MergeExternal(array, lastA, Range(lastA.end, B.end), compare, cache);
						else if (buffer2.length() > 0)
							MergeInternal(array, lastA, Range(lastA.end, B.end), compare, buffer2);
						else
							MergeInPlace(array, lastA, Range(lastA.end, B.end), compare);
					}
				}

				// when we're finished with this merge step we should have the one or two internal buffers left over, where the second buffer is all jumbled up
				// insertion sort the second buffer, then redistribute the buffers back into the array using the opposite process used for creating the buffer

				// while an unstable sort like std::sort could be applied here, in benchmarks it was consistently slightly slower than a simple insertion sort,
				// even for tens of millions of items. this may be because insertion sort is quite fast when the data is already somewhat sorted, like it is here
				InsertionSort(array, buffer2, compare);

				for (pull_index = 0; pull_index < 2; pull_index++) {
					size_t unique = pull[pull_index].count * 2;
					if (pull[pull_index].from > pull[pull_index].to) {
						// the values were pulled out to the left, so redistribute them back to the right
						Range buffer = Range(pull[pull_index].range.start, pull[pull_index].range.start + pull[pull_index].count);
						while (buffer.length() > 0) {
							index = FindFirstForward(array, array[buffer.start], Range(buffer.end, pull[pull_index].range.end), compare, unique);
							size_t amount = index - buffer.end;
							Rotate(array, buffer.length(), Range(buffer.start, index));
							buffer.start += (amount + 1);
							buffer.end += amount;
							unique -= 2;
						}
					} else if (pull[pull_index].from < pull[pull_index].to) {
						// the values were pulled out to the right, so redistribute them back to the left
						Range buffer = Range(pull[pull_index].range.end - pull[pull_index].count, pull[pull_index].range.end);
						while (buffer.length() > 0) {
							index = FindLastBackward(array, array[buffer.end - 1], Range(pull[pull_index].range.start, buffer.start), compare, unique);
							size_t amount = buffer.start - index;
							Rotate(array, amount, Range(index, buffer.end));
							buffer.start -= amount;
							buffer.end -= (amount + 1);
							unique -= 2;
						}
					}
				}
			}

			// double the size of each A and B subarray that will be merged in the next level
			if (!iterator.nextLevel()) break;
		}
	}
}




// class to test stable sorting (index will contain its original index in the array, to make sure it doesn't switch places with other items)
class Test {
public:
	size_t value;
#if VERIFY
	size_t index;
#endif

#if PROFILE
	Test& operator=(const Test & rhs) {
		assignments++;
		value = rhs.value;
		#if VERIFY
			index = rhs.index;
		#endif
		return *this;
	}
#endif
};

#if SLOW_COMPARISONS
	#define NOOP_SIZE 50
	size_t noop1[NOOP_SIZE], noop2[NOOP_SIZE];
#endif

bool TestCompare(Test item1, Test item2) {
	#if PROFILE
		comparisons++;
	#endif

	#if SLOW_COMPARISONS
		// test slow comparisons by adding some fake overhead
		// (in real-world use this might be string comparisons, etc.)
		for (size_t index = 0; index < NOOP_SIZE; index++)
			noop1[index] = noop2[index];
	#endif

	return (item1.value < item2.value);
}


using namespace std;

// make sure the items within the given range are in a stable order
// if you want to test the correctness of any changes you make to the main WikiSort function,
// move this function to the top of the file and call it from within WikiSort after each step
#if VERIFY
template <typename Comparison>
void Verify(const Test array[], const Range range, const Comparison compare, const string msg) {
	for (size_t index = range.start + 1; index < range.end; index++) {
		// if it's in ascending order then we're good
		// if both values are equal, we need to make sure the index values are ascending
		if (!(compare(array[index - 1], array[index]) ||
			  (!compare(array[index], array[index - 1]) && array[index].index > array[index - 1].index))) {

			//for (size_t index2 = range.start; index2 < range.end; index2++)
			//	cout << array[index2].value << " (" << array[index2].index << ") ";

			cout << endl << "failed with message: " << msg << endl;
			assert(false);
		}
	}
}
#endif

namespace Testing {
	size_t Random(size_t index, size_t total) {
		return rand();
	}

	size_t RandomFew(size_t index, size_t total) {
		return rand() * (100.0/RAND_MAX);
	}

	size_t MostlyDescending(size_t index, size_t total) {
		return total - index + rand() * 1.0/RAND_MAX * 5 - 2.5;
	}

	size_t MostlyAscending(size_t index, size_t total) {
		return index + rand() * 1.0/RAND_MAX * 5 - 2.5;
	}

	size_t Ascending(size_t index, size_t total) {
		return index;
	}

	size_t Descending(size_t index, size_t total) {
		return total - index;
	}

	size_t Equal(size_t index, size_t total) {
		return 1000;
	}

	size_t Jittered(size_t index, size_t total) {
		return (rand() * 1.0/RAND_MAX <= 0.9) ? index : (index - 2);
	}

	size_t MostlyEqual(size_t index, size_t total) {
		return 1000 + rand() * 1.0/RAND_MAX * 4;
	}

	// the last 1/5 of the data is random
	size_t Append(size_t index, size_t total) {
		if (index > total - total/5) return rand() * 1.0/RAND_MAX * total;
		return index;
	}
}

int main() {
	const size_t max_size = 1500000;
	__typeof__(&TestCompare) compare = &TestCompare;
	vector<Test> array1, array2;

	#if PROFILE
		size_t compares1, compares2, total_compares1 = 0, total_compares2 = 0;
		size_t assigns1, assigns2, total_assigns1 = 0, total_assigns2 = 0;
	#endif

	// initialize the random-number generator
	//srand(time(NULL));
	srand(10141985); // in case you want the same random numbers

	size_t total = max_size;

#if !SLOW_COMPARISONS && VERIFY
	__typeof__(&Testing::Random) test_cases[] = {
		Testing::Random,
		Testing::RandomFew,
		Testing::MostlyDescending,
		Testing::MostlyAscending,
		Testing::Ascending,
		Testing::Descending,
		Testing::Equal,
		Testing::Jittered,
		Testing::MostlyEqual,
		Testing::Append
	};

	cout << "running test cases... " << flush;
	array1.resize(total);
	array2.resize(total);
	for (int test_case = 0; test_case < sizeof(test_cases)/sizeof(test_cases[0]); test_case++) {
		for (size_t index = 0; index < total; index++) {
			Test item = Test();
			item.value = test_cases[test_case](index, total);
			item.index = index;

			array1[index] = array2[index] = item;
		}
		Wiki::Sort(array1.begin(), array1.end(), compare);
		stable_sort(array2.begin(), array2.end(), compare);

		Verify(&array1[0], Range(0, total), compare, "test case failed");
		for (size_t index = 0; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
	}
	cout << "passed!" << endl;
#endif

	double total_time = Seconds();
	double total_time1 = 0, total_time2 = 0;

	for (total = 0; total <= max_size; total += 2048 * 16) {
		array1.resize(total);
		array2.resize(total);

		for (size_t index = 0; index < total; index++) {
			Test item = Test();

			// Random, RandomFew, MostlyDescending, MostlyAscending,
			// Ascending, Descending, Equal, Jittered, MostlyEqual, Append
			item.value = Testing::Random(index, total);
			#if VERIFY
				item.index = index;
			#endif

			array1[index] = array2[index] = item;
		}

		double time1 = Seconds();
		#if PROFILE
			comparisons = assignments = 0;
		#endif
		Wiki::Sort(array1.begin(), array1.end(), compare);
		time1 = Seconds() - time1;
		total_time1 += time1;

		#if PROFILE
			compares1 = comparisons;
			total_compares1 += compares1;
			assigns1 = assignments;
			total_assigns1 += assigns1;
		#endif

		double time2 = Seconds();
		#if PROFILE
			comparisons = assignments = 0;
		#endif

		#if TEST_INPLACE
			__inplace_stable_sort(array2.begin(), array2.end(), compare);
		#else
			stable_sort(array2.begin(), array2.end(), compare);
		#endif
		time2 = Seconds() - time2;
		total_time2 += time2;

		#if PROFILE
			compares2 = comparisons;
			total_compares2 += compares2;
			assigns2 = assignments;
			total_assigns2 += assigns2;
		#endif

		cout << "[" << total << "]" << endl;

		if (time1 >= time2) cout << "WikiSort: " << time1 << " seconds, stable_sort: " << time2 << " seconds (" << time2/time1 * 100.0 << "% as fast)" << endl;
		else cout << "WikiSort: " << time1 << " seconds, stable_sort: " << time2 << " seconds (" << time2/time1 * 100.0 - 100.0 << "% faster)" << endl;

		#if PROFILE
			if (compares1 <= compares2) cout << "WikiSort: " << compares1 << " compares, stable_sort: " << compares2 << " compares (" << compares1 * 100.0/compares2 << "% as many)" << endl;
			else cout << "WikiSort: " << compares1 << " compares, stable_sort: " << compares2 << " compares (" << compares1 * 100.0/compares2 - 100.0 << "% more)" << endl;

			if (assigns1 <= assigns2) cout << "WikiSort: " << assigns1 << " assigns, stable_sort: " << assigns2 << " assigns (" << assigns1 * 100.0/assigns2 << "% as many)" << endl;
			else cout << "WikiSort: " << assigns1 << " assigns, stable_sort: " << assigns2 << " assigns (" << assigns1 * 100.0/assigns2 - 100.0 << "% more)" << endl;
		#endif

		#if VERIFY
			// make sure the arrays are sorted correctly, and that the results were stable
			cout << "verifying... " << flush;

			Verify(&array1[0], Range(0, total), compare, "testing the final array");
			for (size_t index = 0; index < total; index++)
				assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));

			cout << "correct!" << endl;
		#endif
	}

	total_time = Seconds() - total_time;
	cout << "Tests completed in " << total_time << " seconds" << endl;
	if (total_time1 >= total_time2) cout << "WikiSort: " << total_time1 << " seconds, stable_sort: " << total_time2 << " seconds (" << total_time2/total_time1 * 100.0 << "% as fast)" << endl;
	else cout << "WikiSort: " << total_time1 << " seconds, stable_sort: " << total_time2 << " seconds (" << total_time2/total_time1 * 100.0 - 100.0 << "% faster)" << endl;

	#if PROFILE
		if (total_compares1 <= total_compares2) cout << "WikiSort: " << total_compares1 << " compares, stable_sort: " << total_compares2 << " compares (" << total_compares1 * 100.0/total_compares2 << "% as many)" << endl;
		else cout << "WikiSort: " << total_compares1 << " compares, stable_sort: " << total_compares2 << " compares (" << total_compares1 * 100.0/total_compares2 - 100.0 << "% more)" << endl;

		if (total_assigns1 <= total_assigns2) cout << "WikiSort: " << total_assigns1 << " assigns, stable_sort: " << total_assigns2 << " assigns (" << total_assigns1 * 100.0/total_assigns2 << "% as many)" << endl;
		else cout << "WikiSort: " << total_assigns1 << " assigns, stable_sort: " << total_assigns2 << " assigns (" << total_assigns1 * 100.0/total_assigns2 - 100.0 << "% more)" << endl;
	#endif

	return 0;
}
