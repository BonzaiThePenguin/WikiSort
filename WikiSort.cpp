/*
 g++ -o WikiSort.x WikiSort.cpp -O3
 ./WikiSort.x
*/

#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <assert.h>

using namespace std;

// if true, Verify() will be called after each merge step to make sure it worked correctly
#define VerifyWhileSorting false


#define Seconds()					(clock() * 1.0/CLOCKS_PER_SEC)
#define Var(name, value)			__typeof__(value) name = value


// structure to represent ranges within the array
typedef struct {
	long start;
	long length;
} Range;

Range MakeRange(const long start, const long length) {
	Range range;
	range.start = start;
	range.length = length;
	return range;
}

Range RangeBetween(const long start, const long end) {
	return MakeRange(start, end - start);
}

Range ZeroRange() {
	return MakeRange(0, 0);
}


// toolbox functions used by the sorter

// if your language does not support bitwise operations for some reason, you can use (floor(value/2) * 2 == value)
bool IsEven(const long value) { return ((value & 0x1) == 0x0); }

// 63 -> 32, 64 -> 64, etc.
// apparently this comes from Hacker's Delight?
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

// find the index of the first value within the range that is equal to array[index]
template <typename T, class Comparison>
long BinaryFirst(const T array[], const long index, const Range range, const Comparison compare) {
	long start = range.start, end = range.start + range.length - 1;
	while (start < end) {
		long mid = start + (end - start)/2;
		if (compare(array[mid], array[index]))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.start + range.length - 1 && compare(array[start], array[index])) start++;
	return start;
}

//template <typename T, class Comparison>
//long BinaryFirst(const vector<T> &array, const long index, const Range range, const Comparison compare) {
//	typename vector<T>::const_iterator first = lower_bound(array.begin() + range.start, array.begin() + range.start + range.length, array[index], compare);
//	return (first - array.begin());
//}

// find the index of the last value within the range that is equal to array[index], plus 1
template <typename T, class Comparison>
long BinaryLast(const T array[], const long index, const Range range, const Comparison compare) {
	long start = range.start, end = range.start + range.length - 1;
	while (start < end) {
		long mid = start + (end - start)/2;
		if (!compare(array[index], array[mid]))
			start = mid + 1;
		else
			end = mid;
	}
	if (start == range.start + range.length - 1 && !compare(array[index], array[start])) start++;
	return start;
}

//template <typename T, class Comparison>
//long BinaryLast(const vector<T> &array, const long index, const Range range, const Comparison compare) {
//	typename vector<T>::const_iterator last = upper_bound(array.begin() + range.start, array.begin() + range.start + range.length, array[index], compare);
//	return (last - array.begin());
//}

// n^2 sorting algorithm used to sort tiny chunks of the full array
template <typename T, class Comparison>
void InsertionSort(T array[], const Range range, const Comparison compare) {
	for (long i = range.start + 1; i < range.start + range.length; i++) {
		const T temp = array[i]; long j;
		for (j = i; j > range.start && compare(temp, array[j - 1]); j--)
			array[j] = array[j - 1];
		array[j] = temp;
	}
}

// reverse a range within the array
// reverse(&array[range.start], &array[range.start + range.length]);
template <typename T>
void Reverse(T array[], const Range range) {
	for (long index = range.length/2 - 1; index >= 0; index--)
		swap(array[range.start + index], array[range.start + range.length - index - 1]);
}

// swap a series of values in the array
// swap_ranges(&array[start1], &array[start1 + block_size], &array[start2]);
template <typename T>
void BlockSwap(T array[], const long start1, const long start2, const long block_size) {
	for (long index = 0; index < block_size; index++)
		swap(array[start1 + index], array[start2 + index]);
}

// rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1)
template <typename T>
void Rotate(T array[], const long amount, const Range range, T cache[], const long cache_size) {
	if (range.length == 0) return;
	
	long split;
	if (amount >= 0) split = range.start + (amount % range.length);
	else split = range.start + range.length - ((-amount) % range.length);
	
	Range range1 = RangeBetween(range.start, split);
	Range range2 = RangeBetween(split, range.start + range.length);
	
	// if the smaller of the two ranges fits into the cache, it's *slightly* faster copying it there and shifting the elements over
	if (range1.length <= range2.length) {
		if (range1.length <= cache_size) {
			memcpy(&cache[0], &array[range1.start], range1.length * sizeof(array[0]));
			memmove(&array[range1.start], &array[range2.start], range2.length * sizeof(array[0]));
			memcpy(&array[range1.start + range2.length], &cache[0], range1.length * sizeof(array[0]));
			return;
		}
	} else {
		if (range2.length <= cache_size) {
			memcpy(&cache[0], &array[range2.start], range2.length * sizeof(array[0]));
			memmove(&array[range2.start + range2.length - range1.length], &array[range1.start], range1.length * sizeof(array[0]));
			memcpy(&array[range1.start], &cache[0], range2.length * sizeof(array[0]));
			return;
		}
	}
	
	//rotate(&array[range.start], &array[split], &array[range.start + range.length]);
	Reverse(array, range1);
	Reverse(array, range2);
	Reverse(array, range);
}

// make sure the items within the given range are in a stable order
template <typename T, class Comparison>
void Verify(const T array[], const Range range, const Comparison compare, const string msg) {
	for (long index = range.start + 1; index < range.start + range.length; index++) {
		if (!(compare(array[index - 1], array[index]) || (!compare(array[index], array[index - 1]) && array[index].index > array[index - 1].index))) {
			for (long index2 = range.start; index2 < range.start + range.length; index2++) cout << array[index2].value << " (" << array[index2].index << ") ";
			cout << endl << "failed with message: " << msg << endl;
			assert(false);
		}
	}
}

// standard merge operation using an internal buffer
template <typename T, class Comparison>
void WikiMerge(T array[], const Range buffer, const Range A, const Range B, const Comparison compare, T cache[], const long cache_size) {
	if (B.length <= 0 || A.length <= 0) return;
	if (!compare(array[B.start], array[A.start + A.length - 1])) return;
	
	// if A fits into the cache, use that instead of the internal buffer
	if (A.length <= cache_size) {
		// copy the rest of A into the buffer
		memcpy(&cache[0], &array[A.start], A.length * sizeof(array[0]));
		
		long A_count = 0, B_count = 0, insert = 0;
		while (A_count < A.length && B_count < B.length) {
			if (!compare(array[B.start + B_count], cache[A_count])) {
				array[A.start + insert] = cache[A_count];
				A_count++;
			} else {
				array[A.start + insert] = array[B.start + B_count];
				B_count++;
			}
			insert++;
		}
		
		// copy the remainder of A into the final array
		memcpy(&array[A.start + insert], &cache[A_count], (A.length - A_count) * sizeof(array[0]));
	} else {
		// swap the rest of A into the buffer
		BlockSwap(array, buffer.start, A.start, A.length);
		
		// whenever we find a value to add to the final array, swap it with the value that's already in that spot
		// when this algorithm is finished, 'buffer' will contain its original contents, but in a different order
		long A_count = 0, B_count = 0, insert = 0;
		while (A_count < A.length && B_count < B.length) {
			if (!compare(array[B.start + B_count], array[buffer.start + A_count])) {
				swap(array[A.start + insert], array[buffer.start + A_count]);
				A_count++;
			} else {
				swap(array[A.start + insert], array[B.start + B_count]);
				B_count++;
			}
			insert++;
		}
		
		// swap the remainder of A into the final array
		BlockSwap(array, buffer.start + A_count, A.start + insert, A.length - A_count);
	}
}

// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
template <typename T, class Comparison>
void WikiSort(vector<T> &vec, const Comparison compare) {
	// switch over to a C-array, as it runs faster and we never resize the vector
	T *array = &vec[0];
	const long size = vec.size();
	
	// reverse any descending ranges in the array, as that will allow them to sort faster
	Range reverse = ZeroRange();
	for (long index = 1; index < size; index++) {
		if (compare(array[index], array[index - 1])) reverse.length++;
		else { Reverse(array, reverse); reverse = MakeRange(index, 0); }
	}
	Reverse(array, reverse);
	
	if (size <= 32) {
		// insertion sort the array, as there are 32 or fewer items
		InsertionSort(array, RangeBetween(0, size), compare);
		return;
	}
	
	// use a small cache to speed up some of the operations
	const long cache_size = 200;
	T cache[cache_size];
	
	// calculate how to scale the index value to the range within the array
	const long power_of_two = FloorPowerOfTwo(size);
	double scale = size/(double)power_of_two; // 1.0 <= scale < 2.0
	
	// first insertion sort everything the lowest level, which is 16-31 items at a time
	for (long merge_index = 0; merge_index < power_of_two; merge_index += 16) {
		long start = merge_index * scale;
		long end = (merge_index + 16) * scale;
		InsertionSort(array, RangeBetween(start, end), compare);
	}
	
	// then merge sort the higher levels, which can be 32-63, 64-127, 128-255, etc.
	for (long merge_size = 16; merge_size < power_of_two; merge_size += merge_size) {
		long block_size = max((long)sqrt(merge_size * scale), (long)3);
		long buffer_size = (merge_size * scale)/block_size + 1;
		
		// as an optimization, we really only need to pull out an internal buffer once for each level of merges
		// after that we can reuse the same buffer over and over, then redistribute it when we're finished with this level
		Range level1 = ZeroRange(), level2, levelA, levelB;
		
		for (long merge_index = 0; merge_index < power_of_two - merge_size; merge_index += merge_size + merge_size) {
			// the floating-point multiplication here is consistently about 10% faster than using min(merge_index + merge_size + merge_size, size),
			// probably because the overhead of the multiplication is offset by guaranteeing evenly sized subarrays, which is optimal
			long start = merge_index * scale;
			long mid = (merge_index + merge_size) * scale;
			long end = (merge_index + merge_size + merge_size) * scale;
			
			if (compare(array[end - 1], array[start])) {
				// the two ranges are in reverse order, so a simple rotation should fix it
				Rotate(array, mid - start, RangeBetween(start, end), cache, cache_size);
				if (VerifyWhileSorting) Verify(array, RangeBetween(start, end), compare, "reversing order via Rotate()");
			} else if (compare(array[mid], array[mid - 1])) {
				// these two ranges weren't already in order, so we'll need to merge them!
				Range A = RangeBetween(start, mid), B = RangeBetween(mid, end);
				
				if (VerifyWhileSorting) Verify(array, A, compare, "making sure A is valid");
				if (VerifyWhileSorting) Verify(array, B, compare, "making sure B is valid");
				
				// try to fill up two buffers with unique values in ascending order
				Range bufferA, bufferB, buffer1, buffer2, blockA, blockB, firstA, lastA, lastB;
				
				if (A.length <= cache_size) {
					WikiMerge(array, ZeroRange(), A, B, compare, cache, cache_size);
					if (VerifyWhileSorting) Verify(array, RangeBetween(A.start, B.start + B.length), compare, "using the cache to merge A and B");
					continue;
				}
				
				if (level1.length > 0) {
					// reuse the buffers we found in a previous iteration
					bufferA = MakeRange(A.start, 0);
					bufferB = MakeRange(B.start + B.length, 0);
					buffer1 = level1;
					buffer2 = level2;
				} else {
					// the first item is always going to be the first unique value, so let's start searching at the next index
					buffer1.length = 1;
					for (buffer1.start = A.start + 1; buffer1.start < A.start + A.length; buffer1.start++) {
						if (compare(array[buffer1.start - 1], array[buffer1.start]) || compare(array[buffer1.start], array[buffer1.start - 1])) {
							buffer1.length++;
							if (buffer1.length == buffer_size) break;
						}
					}
					
					// if the size of each block fits into the cache, we only need one buffer for tagging the A blocks
					// this is because the other buffer is used as a swap space for merging the A blocks into the B values that follow it,
					// but we can just use the cache as the buffer instead. this skips some memmoves and an insertion sort
					if (buffer_size <= cache_size) {
						buffer2 = MakeRange(A.start, 0);
						
						if (buffer1.length == buffer_size) {
							// we found enough values for the buffer in A
							bufferA = MakeRange(buffer1.start, buffer_size);
							bufferB = MakeRange(B.start + B.length, 0);
							buffer1 = MakeRange(A.start, buffer_size);
						} else {
							// we were unable to find enough unique values in A, so try B
							bufferA = MakeRange(buffer1.start, 0);
							buffer1 = MakeRange(A.start, 0);
							
							// the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index
							buffer1.length = 1;
							for (buffer1.start = B.start + B.length - 2; buffer1.start >= B.start; buffer1.start--) {
								if (compare(array[buffer1.start], array[buffer1.start + 1]) || compare(array[buffer1.start + 1], array[buffer1.start])) {
									buffer1.length++;
									if (buffer1.length == buffer_size) break;
								}
							}
							
							if (buffer1.length == buffer_size) {
								bufferB = MakeRange(buffer1.start, buffer_size);
								buffer1 = MakeRange(B.start + B.length - buffer_size, buffer_size);
							}
						}
					} else {
						// the first item of the second buffer isn't guaranteed to be the first unique value, so we need to find the first unique item too
						buffer2.length = 0;
						for (buffer2.start = buffer1.start + 1; buffer2.start < A.start + A.length; buffer2.start++) {
							if (compare(array[buffer2.start - 1], array[buffer2.start]) || compare(array[buffer2.start], array[buffer2.start - 1])) {
								buffer2.length++;
								if (buffer2.length == buffer_size) break;
							}
						}
						
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
							
							// like before, the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index
							buffer2.length = 1;
							for (buffer2.start = B.start + B.length - 2; buffer2.start >= B.start; buffer2.start--) {
								if (compare(array[buffer2.start], array[buffer2.start + 1]) || compare(array[buffer2.start + 1], array[buffer2.start])) {
									buffer2.length++;
									if (buffer2.length == buffer_size) break;
								}
							}
							
							if (buffer2.length == buffer_size) {
								bufferB = MakeRange(buffer2.start, buffer_size);
								buffer2 = MakeRange(B.start + B.length - buffer_size, buffer_size);
							} else buffer1.length = 0; // failure
						} else {
							// we were unable to find a single buffer in A, so we'll need to find two buffers in B
							buffer1.length = 1;
							for (buffer1.start = B.start + B.length - 2; buffer1.start >= B.start; buffer1.start--) {
								if (compare(array[buffer1.start], array[buffer1.start + 1]) || compare(array[buffer1.start + 1], array[buffer1.start])) {
									buffer1.length++;
									if (buffer1.length == buffer_size) break;
								}
							}
							
							buffer2.length = 0;
							for (buffer2.start = buffer1.start - 1; buffer2.start >= B.start; buffer2.start--) {
								if (compare(array[buffer2.start], array[buffer2.start + 1]) || compare(array[buffer2.start + 1], array[buffer2.start])) {
									buffer2.length++;
									if (buffer2.length == buffer_size) break;
								}
							}
							
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
						Range oldA = A, oldB = B;
						
						while (A.length > 0 && B.length > 0) {
							// find the first place in B where the first item in A needs to be inserted
							long mid = BinaryFirst(array, A.start, B, compare);
							
							// rotate A into place
							long amount = mid - (A.start + A.length);
							Rotate(array, -amount, RangeBetween(A.start, mid), cache, cache_size);
							
							// calculate the new A and B ranges
							B = RangeBetween(mid, B.start + B.length);
							A = RangeBetween(BinaryLast(array, A.start + amount, A, compare), B.start);
						}
						
						if (VerifyWhileSorting) Verify(array, RangeBetween(oldA.start, oldB.start + oldB.length), compare, "performing a brute-force in-place merge");
						continue;
					}
					
					// move the unique values to the start of A if needed
					for (long index = bufferA.start, count = 0; count < bufferA.length; index--) {
						if (index == A.start || compare(array[index - 1], array[index]) || compare(array[index], array[index - 1])) {
							Rotate(array, -count, RangeBetween(index + 1, bufferA.start + 1), cache, cache_size);
							bufferA.start = index + count; count++;
						}
					}
					bufferA.start = A.start;
					
					if (VerifyWhileSorting) {
						Verify(array, MakeRange(A.start, bufferA.length), compare, "testing values pulled out from A");
						Verify(array, RangeBetween(A.start + bufferA.length, A.start + A.length), compare, "testing remainder of A");
					}
					
					// move the unique values to the end of B if needed
					for (long index = bufferB.start, count = 0; count < bufferB.length; index++) {
						if (index == B.start + B.length - 1 || compare(array[index], array[index + 1]) || compare(array[index + 1], array[index])) {
							Rotate(array, count, RangeBetween(bufferB.start, index), cache, cache_size);
							bufferB.start = index - count; count++;
						}
					}
					bufferB.start = B.start + B.length - bufferB.length;
					
					if (VerifyWhileSorting) {
						Verify(array, MakeRange(B.start + B.length - bufferB.length, bufferB.length), compare, "testing values pulled out from B");
						Verify(array, RangeBetween(B.start, B.start + B.length - bufferB.length), compare, "testing remainder of B");
					}
					
					// reuse these buffers next time!
					level1 = buffer1;
					level2 = buffer2;
					levelA = bufferA;
					levelB = bufferB;
				}
				
				// break the remainder of A into blocks. firstA is the uneven-sized first A block
				blockA = RangeBetween(bufferA.start + bufferA.length, A.start + A.length);
				firstA = MakeRange(bufferA.start + bufferA.length, blockA.length % block_size);
				
				// swap the second value of each A block with the value in buffer1
				for (long index = 0, indexA = firstA.start + firstA.length + 1; indexA < blockA.start + blockA.length; index++, indexA += block_size)
					swap(array[buffer1.start + index], array[indexA]);
				
				// start rolling the A blocks through the B blocks!
				// whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well
				lastA = firstA;
				lastB = ZeroRange();
				blockB = MakeRange(B.start, min(block_size, B.length - bufferB.length));
				blockA.start += firstA.length;
				blockA.length -= firstA.length;
				
				long minA = blockA.start, indexA = 0;
				
				while (true) {
					// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block
					if ((lastB.length > 0 && !compare(array[lastB.start + lastB.length - 1], array[minA])) || blockB.length == 0) {
						// figure out where to split the previous B block, and rotate it at the split
						long B_split = BinaryFirst(array, minA, lastB, compare);
						long B_remaining = lastB.start + lastB.length - B_split;
						
						// swap the minimum A block to the beginning of the rolling A blocks
						BlockSwap(array, blockA.start, minA, block_size);
						
						// we need to swap the second item of the previous A block back with its original value, which is stored in buffer1
						// since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized
						swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
						
						// now we need to split the previous B block at B_split and insert the minimum A block in-between the two parts, using a rotation
						Rotate(array, B_remaining, RangeBetween(B_split, blockA.start + block_size), cache, cache_size);
						
						// locally merge the previous A block with the B values that follow it, using the buffer as swap space
						WikiMerge(array, buffer2, lastA, RangeBetween(lastA.start + lastA.length, B_split), compare, cache, cache_size);
						
						// now we need to update the ranges and stuff
						lastA = MakeRange(blockA.start - B_remaining, block_size);
						lastB = MakeRange(lastA.start + lastA.length, B_remaining);
						blockA.start += block_size; blockA.length -= block_size;
						if (blockA.length == 0) break;
						
						// search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!)
						minA = blockA.start + 1;
						for (long findA = minA + block_size; findA < blockA.start + blockA.length; findA += block_size)
							if (compare(array[findA], array[minA])) minA = findA;
						minA = minA - 1; // decrement once to get back to the start of that A block
					} else if (blockB.length < block_size) {
						// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
						Rotate(array, -blockB.length, RangeBetween(blockA.start, blockB.start + blockB.length), cache, cache_size);
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
				WikiMerge(array, buffer2, lastA, RangeBetween(lastA.start + lastA.length, B.start + B.length - bufferB.length), compare, cache, cache_size);
				
				// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
				// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
				InsertionSort(array, buffer2, compare);
				
				if (VerifyWhileSorting) Verify(array, RangeBetween(A.start + bufferA.length, B.start + B.length - bufferB.length), compare, "making sure the local merges worked");
			}
		}
		
		if (level1.length > 0) {
			// redistribute bufferA back into the array
			long level_start = levelA.start;
			for	(long index = levelA.start + levelA.length; levelA.length > 0; index++) {
				if (index == levelB.start || !compare(array[index], array[levelA.start])) {
					long amount = index - (levelA.start + levelA.length);
					Rotate(array, -amount, RangeBetween(levelA.start, index), cache, cache_size);
					levelA.start += amount + 1; levelA.length--; index--;
				}
			}
			if (VerifyWhileSorting) Verify(array, RangeBetween(level_start, levelB.start), compare, "redistributed levelA back into the array");
			
			// redistribute levelB back into the array
			long level_end = levelB.start + levelB.length;
			for (long index = levelB.start; levelB.length > 0; index--) {
				if (index == level_start || !compare(array[levelB.start + levelB.length - 1], array[index - 1])) {
					long amount = levelB.start - index;
					Rotate(array, amount, RangeBetween(index, levelB.start + levelB.length), cache, cache_size);
					levelB.start -= amount; levelB.length--; index++;
				}
			}
			if (VerifyWhileSorting) Verify(array, RangeBetween(level_start, level_end), compare, "redistributed levelB back into the array");
		}
	}
}


// structure to test stable sorting (index will contain its original index in the array, to make sure it doesn't switch places with other items)
typedef struct {
	int value;
	int index;
} Test;

bool TestCompare(Test item1, Test item2) { return (item1.value < item2.value); }

int main() {
	const long max_size = 1500000;
	Var(compare, &TestCompare);
	vector<Test> array1, array2;
	
	// initialize the random-number generator
	srand(/*time(NULL)*/ 10141985);
	
	double total_time = Seconds();
	
	for (long total = 0; total < max_size; total += 2048 * 16) {
		array1.resize(total);
		array2.resize(total);
		
		for (long index = 0; index < total; index++) {
			Test item; item.index = index;
			
			// here are some possible tests to perform on this sorter:
			// if (index == 0) item.value = 10;
			// else if (index < total/2) item.value = 11;
			// else if (index == total - 1) item.value = 10;
			// else item.value = 9;
			// item.value = rand();
			// item.value = total - index + rand() * 1.0/RAND_MAX * 5 - 2.5;
			// item.value = index + rand() * 1.0/RAND_MAX * 5 - 2.5;
			// item.value = index;
			// item.value = total - index;
			// item.value = 1000;
			// item.value = (rand() * 1.0/RAND_MAX <= 0.9) ? index : (index - 2);
			// item.value = 1000 + rand() * 1.0/RAND_MAX * 4;
			
			item.value = rand();
			
			array1[index] = array2[index] = item;
		}
		
		double time1 = Seconds();
		WikiSort(array1, compare);
		time1 = Seconds() - time1;
		
		double time2 = Seconds();
		stable_sort(array2.begin(), array2.end(), compare);
		time2 = Seconds() - time2;
		
		cout << "[" << total << "] wiki: " << time1 << ", merge: " << time2 << " (" << time2/time1 * 100.0 << "%)" << endl;
		
		// make sure the arrays are sorted correctly, and that the results were stable
		cout << "verifying... " << flush;
		Verify(&array1[0], MakeRange(0, total), compare, "testing the final array");
		if (total > 0) assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (long index = 1; index < total; index++) assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		cout << "correct!" << endl;
	}
	
	total_time = Seconds() - total_time;
	cout << "tests completed in " << total_time << " seconds" << endl;
	
	return 0;
}
