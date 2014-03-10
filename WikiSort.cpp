/*
 clang++ -o WikiSort.x WikiSort.cpp -O3
 (or replace 'clang++' with 'g++')
 ./WikiSort.x
*/

#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <assert.h>

using namespace std;

// if true, Wiki::Verify() will be called after each merge step to make sure it worked correctly
#define VERIFY false
#define PROFILE false

double reverse_time, insertion_time, scale_time, rotate_time, merge_time2, merge_time3, min_time, pull_out_time, merge_time, insertion_time2, redistribute_time;
double Seconds() { return clock() * 1.0/CLOCKS_PER_SEC; }

namespace Wiki {
	// structure to represent ranges within the array
	typedef struct {
		long start;
		long end;
		
		inline long length() const { return end - start; }
	} Range;
	
	Range MakeRange(const long start, const long end) throw() {
		Range range;
		range.start = start;
		range.end = end;
		return range;
	}
	
	// toolbox functions used by the sorter
	
	// 63 -> 32, 64 -> 64, etc.
	// apparently this comes from Hacker's Delight?
	long FloorPowerOfTwo (const long value) throw() {
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
	template <typename T, typename Comparison>
	long BinaryFirst(const T array[], const long index, const Range range, const Comparison compare) throw() {
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
	
	//template <typename T, typename Comparison>
	//long BinaryFirst(const vector<T> &array, const long index, const Range range, const Comparison compare) throw() {
	//	typename vector<T>::const_iterator first = lower_bound(array.begin() + range.start, array.begin() + range.end, array[index], compare);
	//	return (first - array.begin());
	//}
	
	// find the index of the last value within the range that is equal to array[index], plus 1
	template <typename T, typename Comparison>
	long BinaryLast(const T array[], const long index, const Range range, const Comparison compare) throw() {
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
	
	//template <typename T, typename Comparison>
	//long BinaryLast(const vector<T> &array, const long index, const Range range, const Comparison compare) throw() {
	//	typename vector<T>::const_iterator last = upper_bound(array.begin() + range.start, array.begin() + range.end, array[index], compare);
	//	return (last - array.begin());
	//}
	
	// n^2 sorting algorithm used to sort tiny chunks of the full array
	template <typename T, typename Comparison>
	void InsertionSort(T array[], const Range range, const Comparison compare) throw() {
		for (long i = range.start + 1; i < range.end; i++) {
			const T temp = array[i]; long j;
			for (j = i; j > range.start && compare(temp, array[j - 1]); j--)
				array[j] = array[j - 1];
			array[j] = temp;
		}
	}
	
	// reverse a range within the array
	// reverse(&array[range.start], &array[range.end]);
	template <typename T>
	void Reverse(T array[], const Range range) throw() {
		for (long index = range.length()/2 - 1; index >= 0; index--)
			swap(array[range.start + index], array[range.end - index - 1]);
	}
	
	// swap a series of values in the array
	// swap_ranges(&array[start1], &array[start1 + block_size], &array[start2]);
	template <typename T>
	void BlockSwap(T array[], const long start1, const long start2, const long block_size) throw() {
		for (long index = 0; index < block_size; index++)
			swap(array[start1 + index], array[start2 + index]);
	}
	
	// rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1)
	template <typename T>
	void Rotate(T array[], const long amount, const Range range, T cache[], const long cache_size) throw() {
		if (range.length() == 0) return;
		
		long split;
		if (amount >= 0) split = range.start + amount;
		else split = range.end + amount;
		
		Range range1 = MakeRange(range.start, split);
		Range range2 = MakeRange(split, range.end);
		
		// if the smaller of the two ranges fits into the cache, it's *slightly* faster copying it there and shifting the elements over
		if (range1.length() <= range2.length()) {
			if (range1.length() <= cache_size) {
				memcpy(&cache[0], &array[range1.start], range1.length() * sizeof(array[0]));
				memmove(&array[range1.start], &array[range2.start], range2.length() * sizeof(array[0]));
				memcpy(&array[range1.start + range2.length()], &cache[0], range1.length() * sizeof(array[0]));
				return;
			}
		} else {
			if (range2.length() <= cache_size) {
				memcpy(&cache[0], &array[range2.start], range2.length() * sizeof(array[0]));
				memmove(&array[range2.end - range1.length()], &array[range1.start], range1.length() * sizeof(array[0]));
				memcpy(&array[range1.start], &cache[0], range2.length() * sizeof(array[0]));
				return;
			}
		}
		
		//rotate(&array[range.start], &array[split], &array[range.end]);
		Reverse(array, range1);
		Reverse(array, range2);
		Reverse(array, range);
	}
	
	// make sure the items within the given range are in a stable order
	template <typename T, typename Comparison>
	void Verify(const T array[], const Range range, const Comparison compare, const string msg) throw() {
		for (long index = range.start + 1; index < range.end; index++) {
			if (!(compare(array[index - 1], array[index]) || (!compare(array[index], array[index - 1]) && array[index].index > array[index - 1].index))) {
				for (long index2 = range.start; index2 < range.end; index2++)
					cout << array[index2].value << " (" << array[index2].index << ") ";
				cout << endl << "failed with message: " << msg << endl;
				assert(false);
			}
		}
	}
	
	// standard merge operation using an internal or external buffer
	template <typename T, typename Comparison>
	void Merge(T array[], const Range buffer, const Range A, const Range B, const Comparison compare, T cache[], const long cache_size) throw() {
		if (B.length() <= 0 || A.length() <= 0) return;
		if (!compare(array[B.start], array[A.end - 1])) return;
		
		// if A fits into the cache, use that instead of the internal buffer
		if (A.length() <= cache_size) {
			// copy the rest of A into the buffer
			memcpy(&cache[0], &array[A.start], A.length() * sizeof(array[0]));
			
			long A_count = 0, B_count = 0, insert = 0;
			while (true) {
				if (!compare(array[B.start + B_count], cache[A_count])) {
					array[A.start + insert] = cache[A_count];
					A_count++;
					insert++;
					if (A_count >= A.length()) break;
				} else {
					array[A.start + insert] = array[B.start + B_count];
					B_count++;
					insert++;
					if (B_count >= B.length()) break;
				}
			}
			
			// copy the remainder of A into the final array
			memcpy(&array[A.start + insert], &cache[A_count], (A.length() - A_count) * sizeof(array[0]));
		} else {
			// swap the rest of A into the buffer
			BlockSwap(array, buffer.start, A.start, A.length());
			
			// whenever we find a value to add to the final array, swap it with the value that's already in that spot
			// when this algorithm is finished, 'buffer' will contain its original contents, but in a different order
			long A_count = 0, B_count = 0, insert = 0;
			while (true) {
				if (!compare(array[B.start + B_count], array[buffer.start + A_count])) {
					swap(array[A.start + insert], array[buffer.start + A_count]);
					A_count++;
					insert++;
					if (A_count >= A.length()) break;
				} else {
					swap(array[A.start + insert], array[B.start + B_count]);
					B_count++;
					insert++;
					if (B_count >= B.length()) break;
				}
			}
			
			// swap the remainder of A into the final array
			BlockSwap(array, buffer.start + A_count, A.start + insert, A.length() - A_count);
		}
	}
	
	// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
	template <typename T, typename Comparison>
	void Sort(vector<T> &vec, const Comparison compare) throw() {
		// switch over to a C-array, as it runs faster and we never resize the vector
		T *array = &vec[0];
		const long size = vec.size();
		
		if (PROFILE) reverse_time = insertion_time = scale_time = rotate_time = merge_time2 = merge_time3 = min_time = pull_out_time = merge_time = insertion_time2 = redistribute_time = 0;
		
		// from profiling, these were the most expensive operations:
		// reversing (2%)
		// insertion sorting the lowest level (9%)
		// scaling the ranges (6%)
		// merging with the cache (15%)
		// in-place local merges (45%)
		//  - finding the next minimum A block (10%)
		
		// reverse any descending ranges in the array, as that will allow them to sort faster
		double time;
		if (PROFILE) time = Seconds();
		Range reverse = MakeRange(0, 0);
		for (long index = 1; index < size; index++) {
			if (compare(array[index], array[index - 1])) reverse.end++;
			else { Reverse(array, reverse); reverse = MakeRange(index, index); }
		}
		Reverse(array, reverse);
		if (PROFILE) reverse_time += Seconds() - time;
		
		if (size <= 32) {
			// insertion sort the array, as there are 32 or fewer items
			InsertionSort(array, MakeRange(0, size), compare);
			return;
		}
		
		// use a small cache to speed up some of the operations
		const long cache_size = 200;
		T cache[cache_size];
		
		// calculate how to scale the index value to the range within the array
		const long power_of_two = FloorPowerOfTwo(size);
		const long fractional_base = power_of_two/16;
		long fractional_step = size % fractional_base;
		long decimal_step = size/fractional_base;
		
		if (PROFILE) time = Seconds();
		// first insertion sort everything the lowest level, which is 16-31 items at a time
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
		
		// then merge sort the higher levels, which can be 32-63, 64-127, 128-255, etc.
		for (long merge_size = 16; merge_size < power_of_two; merge_size += merge_size) {
			long block_size = sqrt(decimal_step);
			long buffer_size = decimal_step/block_size + 1;
			
			// as an optimization, we really only need to pull out an internal buffer once for each level of merges
			// after that we can reuse the same buffer over and over, then redistribute it when we're finished with this level
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
					// the two ranges are in reverse order, so a simple rotation should fix it
					if (PROFILE) time = Seconds();
					Rotate(array, mid - start, MakeRange(start, end), cache, cache_size);
					if (VERIFY) Verify(array, MakeRange(start, end), compare, "reversing order via Rotate()");
					if (PROFILE) rotate_time += Seconds() - time;
				} else if (compare(array[mid], array[mid - 1])) {
					// these two ranges weren't already in order, so we'll need to merge them!
					Range A = MakeRange(start, mid), B = MakeRange(mid, end);
					
					if (VERIFY) Verify(array, A, compare, "making sure A is valid");
					if (VERIFY) Verify(array, B, compare, "making sure B is valid");
					
					// try to fill up two buffers with unique values in ascending order
					Range bufferA, bufferB, buffer1, buffer2, blockA, blockB, firstA, lastA, lastB;
					
					if (A.length() <= cache_size) {
						if (PROFILE) time = Seconds();
						Merge(array, MakeRange(0, 0), A, B, compare, cache, cache_size);
						if (VERIFY) Verify(array, MakeRange(A.start, B.end), compare, "using the cache to merge A and B");
						if (PROFILE) merge_time2 += Seconds() - time;
						continue;
					}
					
					if (level1.length() > 0) {
						// reuse the buffers we found in a previous iteration
						bufferA = MakeRange(A.start, A.start);
						bufferB = MakeRange(B.end, B.end);
						buffer1 = level1;
						buffer2 = level2;
					} else {
						if (PROFILE) time = Seconds();
						// the first item is always going to be the first unique value, so let's start searching at the next index
						long count = 1;
						for (buffer1.start = A.start + 1; buffer1.start < A.end; buffer1.start++)
							if (compare(array[buffer1.start - 1], array[buffer1.start]) || compare(array[buffer1.start], array[buffer1.start - 1]))
								if (++count == buffer_size) break;
						buffer1.end = buffer1.start + count;
						
						// if the size of each block fits into the cache, we only need one buffer for tagging the A blocks
						// this is because the other buffer is used as a swap space for merging the A blocks into the B values that follow it,
						// but we can just use the cache as the buffer instead. this skips some memmoves and an insertion sort
						if (buffer_size <= cache_size) {
							buffer2 = MakeRange(A.start, A.start);
							
							if (buffer1.length() == buffer_size) {
								// we found enough values for the buffer in A
								bufferA = MakeRange(buffer1.start, buffer1.start + buffer_size);
								bufferB = MakeRange(B.end, B.end);
								buffer1 = MakeRange(A.start, A.start + buffer_size);
							} else {
								// we were unable to find enough unique values in A, so try B
								bufferA = MakeRange(buffer1.start, buffer1.start);
								buffer1 = MakeRange(A.start, A.start);
								
								// the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index
								count = 1;
								for (buffer1.start = B.end - 2; buffer1.start >= B.start; buffer1.start--)
									if (compare(array[buffer1.start], array[buffer1.start + 1]) || compare(array[buffer1.start + 1], array[buffer1.start]))
										if (++count == buffer_size) break;
								buffer1.end = buffer1.start + count;
								
								if (buffer1.length() == buffer_size) {
									bufferB = MakeRange(buffer1.start, buffer1.start + buffer_size);
									buffer1 = MakeRange(B.end - buffer_size, B.end);
								}
							}
						} else {
							// the first item of the second buffer isn't guaranteed to be the first unique value, so we need to find the first unique item too
							count = 0;
							for (buffer2.start = buffer1.start + 1; buffer2.start < A.end; buffer2.start++)
								if (compare(array[buffer2.start - 1], array[buffer2.start]) || compare(array[buffer2.start], array[buffer2.start - 1]))
									if (++count == buffer_size) break;
							buffer2.end = buffer2.start + count;
							
							if (buffer2.length() == buffer_size) {
								// we found enough values for both buffers in A
								bufferA = MakeRange(buffer2.start, buffer2.start + buffer_size * 2);
								bufferB = MakeRange(B.end, B.end);
								buffer1 = MakeRange(A.start, A.start + buffer_size);
								buffer2 = MakeRange(A.start + buffer_size, A.start + buffer_size * 2);
							} else if (buffer1.length() == buffer_size) {
								// we found enough values for one buffer in A, so we'll need to find one buffer in B
								bufferA = MakeRange(buffer1.start, buffer1.start + buffer_size);
								buffer1 = MakeRange(A.start, A.start + buffer_size);
								
								// like before, the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index
								count = 1;
								for (buffer2.start = B.end - 2; buffer2.start >= B.start; buffer2.start--)
									if (compare(array[buffer2.start], array[buffer2.start + 1]) || compare(array[buffer2.start + 1], array[buffer2.start]))
										if (++count == buffer_size) break;
								buffer2.end = buffer2.start + count;
								
								if (buffer2.length() == buffer_size) {
									bufferB = MakeRange(buffer2.start, buffer2.start + buffer_size);
									buffer2 = MakeRange(B.end - buffer_size, B.end);
								} else buffer1.end = buffer1.start; // failure
							} else {
								// we were unable to find a single buffer in A, so we'll need to find two buffers in B
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
								
								if (buffer2.length() == buffer_size) {
									bufferA = MakeRange(A.start, A.start);
									bufferB = MakeRange(buffer2.start, buffer2.start + buffer_size * 2);
									buffer1 = MakeRange(B.end - buffer_size, B.end);
									buffer2 = MakeRange(buffer1.start - buffer_size, buffer1.start);
								} else buffer1.end = buffer1.start; // failure
							}
						}
						
						if (buffer1.length() < buffer_size) {
							// we failed to fill both buffers with unique values, which implies we're merging two subarrays with a lot of the same values repeated
							// we can use this knowledge to write a merge operation that is optimized for arrays of repeating values
							Range oldA = A, oldB = B;
							
							while (A.length() > 0 && B.length() > 0) {
								// find the first place in B where the first item in A needs to be inserted
								long mid = BinaryFirst(array, A.start, B, compare);
								
								// rotate A into place
								long amount = mid - A.end;
								Rotate(array, -amount, MakeRange(A.start, mid), cache, cache_size);
								
								// calculate the new A and B ranges
								B.start = mid;
								A = MakeRange(BinaryLast(array, A.start + amount, A, compare), B.start);
							}
							
							if (VERIFY) Verify(array, MakeRange(oldA.start, oldB.end), compare, "performing a brute-force in-place merge");
							continue;
						}
						
						// move the unique values to the start of A if needed
						long length = bufferA.length(); count = 0;
						for (long index = bufferA.start; count < length; index--) {
							if (index == A.start || compare(array[index - 1], array[index]) || compare(array[index], array[index - 1])) {
								Rotate(array, -count, MakeRange(index + 1, bufferA.start + 1), cache, cache_size);
								bufferA.start = index + count; count++;
							}
						}
						bufferA = MakeRange(A.start, A.start + length);
						
						if (VERIFY) {
							Verify(array, MakeRange(A.start, A.start + bufferA.length()), compare, "testing values pulled out from A");
							Verify(array, MakeRange(A.start + bufferA.length(), A.end), compare, "testing remainder of A");
						}
						
						// move the unique values to the end of B if needed
						length = bufferB.length(); count = 0;
						for (long index = bufferB.start; count < length; index++) {
							if (index == B.end - 1 || compare(array[index], array[index + 1]) || compare(array[index + 1], array[index])) {
								Rotate(array, count, MakeRange(bufferB.start, index), cache, cache_size);
								bufferB.start = index - count; count++;
							}
						}
						bufferB = MakeRange(B.end - length, B.end);
						
						if (VERIFY) {
							Verify(array, MakeRange(B.end - bufferB.length(), B.end), compare, "testing values pulled out from B");
							Verify(array, MakeRange(B.start, B.end - bufferB.length()), compare, "testing remainder of B");
						}
						
						// reuse these buffers next time!
						level1 = buffer1;
						level2 = buffer2;
						levelA = bufferA;
						levelB = bufferB;
						
						if (PROFILE) pull_out_time += Seconds() - time;
					}
					
					if (PROFILE) time = Seconds();
					
					// break the remainder of A into blocks. firstA is the uneven-sized first A block
					blockA = MakeRange(bufferA.end, A.end);
					firstA = MakeRange(bufferA.end, bufferA.end + blockA.length() % block_size);
					
					// swap the second value of each A block with the value in buffer1
					for (long index = 0, indexA = firstA.end + 1; indexA < blockA.end; index++, indexA += block_size)
						swap(array[buffer1.start + index], array[indexA]);
					
					// start rolling the A blocks through the B blocks!
					// whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well
					lastA = firstA;
					lastB = MakeRange(0, 0);
					blockB = MakeRange(B.start, B.start + min(block_size, B.length() - bufferB.length()));
					blockA.start += firstA.length();
					
					long minA = blockA.start, indexA = 0;
					T min_value = array[minA];
					
					while (true) {
						// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block
						if ((lastB.length() > 0 && !compare(array[lastB.end - 1], min_value)) || blockB.length() == 0) {
							// figure out where to split the previous B block, and rotate it at the split
							long B_split = BinaryFirst(array, minA, lastB, compare);
							long B_remaining = lastB.end - B_split;
							
							// swap the minimum A block to the beginning of the rolling A blocks
							BlockSwap(array, blockA.start, minA, block_size);
							
							// we need to swap the second item of the previous A block back with its original value, which is stored in buffer1
							// since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized
							swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
							
							double time2;
							if (PROFILE) time2 = Seconds();
							
							// now we need to split the previous B block at B_split and insert the minimum A block in-between the two parts, using a rotation
							Rotate(array, B_remaining, MakeRange(B_split, blockA.start + block_size), cache, cache_size);
							
							// locally merge the previous A block with the B values that follow it, using the buffer as swap space
							Merge(array, buffer2, lastA, MakeRange(lastA.end, B_split), compare, cache, cache_size);
							
							if (PROFILE) merge_time3 += Seconds() - time2;
							
							// now we need to update the ranges and stuff
							lastA = MakeRange(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
							lastB = MakeRange(lastA.end, lastA.end + B_remaining);
							blockA.start += block_size;
							if (blockA.length() == 0) break;
							
							if (PROFILE) time2 = Seconds();
							// search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!)
							minA = blockA.start + 1;
							for (long findA = minA + block_size; findA < blockA.end; findA += block_size)
								if (compare(array[findA], array[minA])) minA = findA;
							minA = minA - 1; // decrement once to get back to the start of that A block
							min_value = array[minA];
							if (PROFILE) min_time += Seconds() - time2;
						} else if (blockB.length() < block_size) {
							// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
							Rotate(array, -blockB.length(), MakeRange(blockA.start, blockB.end), cache, cache_size);
							lastB = MakeRange(blockA.start, blockA.start + blockB.length());
							blockA.start += blockB.length();
							blockA.end += blockB.length();
							minA += blockB.length();
							blockB.end = blockB.start;
						} else {
							// roll the leftmost A block to the end by swapping it with the next B block
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
					
					// merge the last A block with the remaining B blocks
					Merge(array, buffer2, lastA, MakeRange(lastA.end, B.end - bufferB.length()), compare, cache, cache_size);
					
					if (PROFILE) merge_time += Seconds() - time;
					
					if (PROFILE) time = Seconds();
					// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
					// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
					InsertionSort(array, buffer2, compare);
					if (PROFILE) insertion_time2 += Seconds() - time;
					
					if (VERIFY) Verify(array, MakeRange(A.start + bufferA.length(), B.end - bufferB.length()), compare, "making sure the local merges worked");
				}
			}
			
			if (level1.length() > 0) {
				if (PROFILE) time = Seconds();
				// redistribute bufferA back into the array
				long level_start = levelA.start;
				for (long index = levelA.end; levelA.length() > 0; index++) {
					if (index == levelB.start || !compare(array[index], array[levelA.start])) {
						long amount = index - (levelA.end);
						Rotate(array, -amount, MakeRange(levelA.start, index), cache, cache_size);
						levelA.start += (amount + 1);
						levelA.end += amount;
						index--;
					}
				}
				if (VERIFY) Verify(array, MakeRange(level_start, levelB.start), compare, "redistributed levelA back into the array");
				
				// redistribute bufferB back into the array
				long level_end = levelB.end;
				for (long index = levelB.start; levelB.length() > 0; index--) {
					if (index == level_start || !compare(array[levelB.end - 1], array[index - 1])) {
						long amount = levelB.start - index;
						Rotate(array, amount, MakeRange(index, levelB.end), cache, cache_size);
						levelB.start -= amount;
						levelB.end -= (amount + 1);
						index++;
					}
				}
				if (VERIFY) Verify(array, MakeRange(level_start, level_end), compare, "redistributed levelB back into the array");
				if (PROFILE) redistribute_time += Seconds() - time;
			}
			
			decimal_step += decimal_step;
			fractional_step += fractional_step;
			if (fractional_step >= fractional_base) {
				fractional_step -= fractional_base;
				decimal_step += 1;
			}
		}
	}
}
					

// structure to test stable sorting (index will contain its original index in the array, to make sure it doesn't switch places with other items)
typedef struct {
	int value;
	int index;
} Test;

bool TestCompare(Test item1, Test item2) { return (item1.value < item2.value); }

namespace Testing {
	long Pathological(long index, long total) {
		if (index == 0) return 10;
		else if (index < total/2) return 11;
		else if (index == total - 1) return 10;
		return 9;
	}
	
	// purely random data is one of the few cases where it is slower than stable_sort(),
	// although it does end up only running at about 70% as fast in that situation
	long Random(long index, long total) {
		return rand();
	}
	
	long MostlyDescending(long index, long total) {
		return total - index + rand() * 1.0/RAND_MAX * 5 - 2.5;
	}
	
	long MostlyAscending(long index, long total) {
		return index + rand() * 1.0/RAND_MAX * 5 - 2.5;
	}
	
	long Ascending(long index, long total) {
		return index;
	}
	
	long Descending(long index, long total) {
		return total - index;
	}
	
	long Equal(long index, long total) {
		return 1000;
	}
	
	long Jittered(long index, long total) {
		return (rand() * 1.0/RAND_MAX <= 0.9) ? index : (index - 2);
	}
	
	long MostlyEqual(long index, long total) {
		return 1000 + rand() * 1.0/RAND_MAX * 4;
	}
}

int main() {
	const long max_size = 1500000;
	__typeof__(&TestCompare) compare = &TestCompare;
	vector<Test> array1, array2;
	
	__typeof__(&Testing::Pathological) test_cases[] = {
		Testing::Pathological,
		Testing::Random,
		Testing::MostlyDescending,
		Testing::MostlyAscending,
		Testing::Ascending,
		Testing::Descending,
		Testing::Equal,
		Testing::Jittered,
		Testing::MostlyEqual
	};
	
	// initialize the random-number generator
	srand(/*time(NULL)*/ 10141985);
	
	cout << "running test cases... " << flush;
	long total = 567;
	array1.resize(total);
	array2.resize(total);
	for (int test_case = 0; test_case < sizeof(test_cases)/sizeof(test_cases[0]); test_case++) {
		for (long index = 0; index < total; index++) {
			Test item;
			
			item.value = test_cases[test_case](index, total);
			item.index = index;
			
			array1[index] = array2[index] = item;
		}
		
		Wiki::Sort(array1, compare);
		stable_sort(array2.begin(), array2.end(), compare);
		
		Wiki::Verify(&array1[0], Wiki::MakeRange(0, total), compare, "test case failed");
		if (total > 0) assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (long index = 1; index < total; index++) assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
	}
	cout << "passed!" << endl;
	
	double total_time = Seconds();
	double total_time1 = 0, total_time2 = 0;
	
	for (total = 0; total < max_size; total += 2048 * 16) {
		array1.resize(total);
		array2.resize(total);
		
		for (long index = 0; index < total; index++) {
			Test item;
			
			item.value = Testing::Random(index, total);
			item.index = index;
			
			array1[index] = array2[index] = item;
		}
		
		double time1 = Seconds();
		Wiki::Sort(array1, compare);
		time1 = Seconds() - time1;
		total_time1 += time1;
		
		double time2 = Seconds();
		//__inplace_stable_sort(array2.begin(), array2.end(), compare);
		stable_sort(array2.begin(), array2.end(), compare);
		time2 = Seconds() - time2;
		total_time2 += time2;
		
		cout << "[" << total << "] wiki: " << time1 << ", merge: " << time2 << " (" << time2/time1 * 100.0 << "%)" << endl;
		if (PROFILE) {
			cout << "reverse: " << reverse_time << " (" << reverse_time/time1 * 100 << "%)" << endl;
			cout << "insert: " << insertion_time << " (" << insertion_time/time1 * 100 << "%)" << endl;
			cout << "scale: " << scale_time << " (" << scale_time/time1 * 100 << "%)" << endl;
			cout << "rotate: " << rotate_time << " (" << rotate_time/time1 * 100 << "%)" << endl;
			cout << "merge2: " << merge_time2 << " (" << merge_time2/time1 * 100 << "%)" << endl;
			cout << "merge3: " << merge_time3 << " (" << merge_time3/time1 * 100 << "%)" << endl;
			cout << "min: " << min_time << " (" << min_time/time1 * 100 << "%)" << endl;
			cout << "pull out: " << pull_out_time << " (" << pull_out_time/time1 * 100 << "%)" << endl;
			cout << "merge: " << merge_time << " (" << merge_time/time1 * 100 << "%)" << endl;
			cout << "insert2: " << insertion_time2 << " (" << insertion_time2/time1 * 100 << "%)" << endl;
			cout << "redistribute: " << redistribute_time << " (" << redistribute_time/time1 * 100 << "%)" << endl;
		}
		
		// make sure the arrays are sorted correctly, and that the results were stable
		cout << "verifying... " << flush;
		Wiki::Verify(&array1[0], Wiki::MakeRange(0, total), compare, "testing the final array");
		if (total > 0) assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (long index = 1; index < total; index++) assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		cout << "correct!" << endl;
	}
	
	total_time = Seconds() - total_time;
	cout << "tests completed in " << total_time << " seconds" << endl;
	cout << "wiki: " << total_time1 << ", merge: " << total_time2 << " (" << total_time2/total_time1 * 100.0 << "%)" << endl;
	
	return 0;
}
