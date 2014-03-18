/***********************************************************
 WikiSort (public domain license)
 https://github.com/BonzaiThePenguin/WikiSort
 
 Reading the documentation on that GitHub page
 is HIGHLY recommended before reading this code!
 
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

double Seconds() { return clock() * 1.0/CLOCKS_PER_SEC; }

// structure to represent ranges within the array
class Range {
public:
	long start;
	long end;
	
	Range() {}
	Range(long start, long end) : start(start), end(end) {}
	inline long length() const { return end - start; }
};

// toolbox functions used by the sorter

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
template <typename T, typename Comparison>
long BinaryFirst(const T array[], const T &value, const Range range, const Comparison compare) {
	return std::lower_bound(&array[range.start], &array[range.end], value, compare) - &array[0];
}

// find the index of the last value within the range that is equal to array[index], plus 1
template <typename T, typename Comparison>
long BinaryLast(const T array[], const T &value, const Range range, const Comparison compare) {
	return std::upper_bound(&array[range.start], &array[range.end], value, compare) - &array[0];
}

// n^2 sorting algorithm used to sort tiny chunks of the full array
template <typename T, typename Comparison>
void InsertionSort(T array[], const Range range, const Comparison compare) {
	std::__insertion_sort(&array[range.start], &array[range.end], compare);
}

// reverse a range within the array
template <typename T>
void Reverse(T array[], const Range range) {
	std::reverse(&array[range.start], &array[range.end]);
}

// swap a series of values in the array
template <typename T>
void BlockSwap(T array[], const long start1, const long start2, const long block_size) {
	std::swap_ranges(&array[start1], &array[start1 + block_size], &array[start2]);
}

// rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1)
template <typename T>
void Rotate(T array[], const long amount, const Range range, T cache[], const long cache_size) {
	if (range.length() == 0) return;
	
	long split;
	if (amount >= 0) split = range.start + amount;
	else split = range.end + amount;
	
	Range range1 = Range(range.start, split);
	Range range2 = Range(split, range.end);
	
	// if the smaller of the two ranges fits into the cache, it's *slightly* faster copying it there and shifting the elements over
	if (range1.length() <= range2.length()) {
		if (range1.length() <= cache_size) {
			std::memcpy(&cache[0], &array[range1.start], range1.length() * sizeof(array[0]));
			std::memmove(&array[range1.start], &array[range2.start], range2.length() * sizeof(array[0]));
			std::memcpy(&array[range1.start + range2.length()], &cache[0], range1.length() * sizeof(array[0]));
			return;
		}
	} else {
		if (range2.length() <= cache_size) {
			std::memcpy(&cache[0], &array[range2.start], range2.length() * sizeof(array[0]));
			std::memmove(&array[range2.end - range1.length()], &array[range1.start], range1.length() * sizeof(array[0]));
			std::memcpy(&array[range1.start], &cache[0], range2.length() * sizeof(array[0]));
			return;
		}
	}
	
	std::rotate(&array[range1.start], &array[range2.start], &array[range2.end]);
}

namespace Wiki {
	// standard merge operation using an internal or external buffer,
	// or if neither are available, use a different in-place merge
	template <typename T, typename Comparison>
	void Merge(T array[], const Range buffer, Range A, Range B, const Comparison compare, T cache[], const long cache_size) {
		if (A.length() <= cache_size) {
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
		} else if (buffer.length() > 0) {
			// whenever we find a value to add to the final array, swap it with the value that's already in that spot
			// when this algorithm is finished, 'buffer' will contain its original contents, but in a different order
			T *A_index = &array[buffer.start], *B_index = &array[B.start];
			T *A_last = &array[buffer.start + A.length()], *B_last = &array[B.end];
			T *insert_index = &array[A.start];
			
			if (B.length() > 0 && A.length() > 0) {
				while (true) {
					if (!compare(*B_index, *A_index)) {
						std::swap(*insert_index, *A_index);
						A_index++;
						insert_index++;
						if (A_index == A_last) break;
					} else {
						std::swap(*insert_index, *B_index);
						B_index++;
						insert_index++;
						if (B_index == B_last) break;
					}
				}
			}
			
			std::swap_ranges(A_index, A_last, insert_index);
		} else {
			// A did not fit into the cache, AND there was no internal buffer available, so we'll need to use a different algorithm entirely
			// this one just repeatedly binary searches into B and rotates A into position,
			// although the paper suggests using the "rotation-based variant of the Hwang and Lin algorithm"
			
			while (A.length() > 0 && B.length() > 0) {
				// find the first place in B where the first item in A needs to be inserted
				long mid = BinaryFirst(array, array[A.start], B, compare);
				
				// rotate A into place
				long amount = mid - A.end;
				Rotate(array, -amount, Range(A.start, mid), cache, 0);
				
				// calculate the new A and B ranges
				B.start = mid;
				A = Range(BinaryLast(array, array[A.start + amount], A, compare), B.start);
			}
		}
	}
	
	// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
	template <typename Iterator, typename Comparison>
	void Sort(Iterator first, Iterator last, const Comparison compare) {
		// map first and last to a C-style array, so we don't have to change the rest of the code
		// (bit of a nasty hack, but it's good enough for now...)
		const long size = last - first;
		__typeof__(&first[0]) array = &first[0];
		
		// if there are 32 or fewer items, just insertion sort the entire array
		if (size <= 32) {
			InsertionSort(array, Range(0, size), compare);
			return;
		}
		
		// use a small cache to speed up some of the operations
		// since the cache size is fixed, it's still O(1) memory!
		// just keep in mind that making it too small ruins the point (nothing will fit into it),
		// and making it too large also ruins the point (so much for "low memory"!)
		// removing the cache entirely still gives 70% of the performance of a standard merge
		const long cache_size = 512;
		__typeof__(array[0]) cache[cache_size];
		
		// also, if you change this to dynamically allocate a full-size buffer,
		// the algorithm seamlessly turns into a full-speed standard merge sort!
		// const long cache_size = (size + 1)/2;
		// etc.
		
		// calculate how to scale the index value to the range within the array
		// (this is essentially fixed-point math, where we manually check for and handle overflow)
		const long power_of_two = FloorPowerOfTwo(size);
		const long fractional_base = power_of_two/16;
		long fractional_step = size % fractional_base;
		long decimal_step = size/fractional_base;
		
		
		// first insertion sort everything the lowest level, which is 16-31 items at a time
		long start, mid, end, decimal = 0, fractional = 0;
		while (decimal < size) {
			start = decimal;
			
			decimal += decimal_step;
			fractional += fractional_step;
			if (fractional >= fractional_base) {
				fractional -= fractional_base;
				decimal++;
			}
			
			end = decimal;
			
			InsertionSort(array, Range(start, end), compare);
		}
		
		
		// then merge sort the higher levels, which can be 32-63, 64-127, 128-255, etc.
		for (long merge_size = 16; merge_size < power_of_two; merge_size += merge_size) {
			
			// if every A and B block will fit into the cache (we use < rather than <= since the block might be one more than decimal_step),
			// use a special branch specifically for merging with the cache
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
					
					if (compare(array[end - 1], array[start])) {
						// the two ranges are in reverse order, so a simple rotation should fix it
						Rotate(array, mid - start, Range(start, end), cache, cache_size);
					} else if (compare(array[mid], array[mid - 1])) {
						// these two ranges weren't already in order, so we'll need to merge them!
						std::copy(&array[start], &array[mid], &cache[0]);
						Merge(array, Range(0, 0), Range(start, mid), Range(mid, end), compare, cache, cache_size);
					}
				}
			} else {
				// this is where the in-place merge logic starts!
				
				// as a reminder (you read the documentation, right? :P), here's what it must do:
				// 1. pull out two internal buffers containing √A unique values
				// 2. loop over the A and B areas within this level of the merge sort
				//     3. break A and B into blocks of size 'block_size'
				//     4. "tag" each of the A blocks with values from the first internal buffer
				//     5. roll the A blocks through the B blocks and drop/rotate them where they belong
				//     6. merge each A block with any B values that follow, using the cache or second the internal buffer
				// 7. sort the second internal buffer if it exists
				// 8. redistribute the two internal buffers back into the array
				
				long block_size = sqrt(decimal_step);
				long buffer_size = decimal_step/block_size + 1;
				
				// as an optimization, we really only need to pull out the internal buffers once for each level of merges
				// after that we can reuse the same buffers over and over, then redistribute it when we're finished with this level
				Range buffer1 = Range(0, 0), buffer2 = Range(0, 0);
				long index, last, count, pull_index = 0, find = buffer_size + buffer_size;
				struct { long from, to, count; Range range; } pull[2] = { { 0 }, { 0 } };
				
				// we need to find either a single contiguous space containing 2√A unique values (which will be split up into two buffers of size √A each),
				// or we need to find one buffer of < 2√A unique values, and a second buffer of √A unique values,
				// OR if we couldn't find that many unique values, we need the largest possible buffer we can get
				
				// in the case where it couldn't find a single buffer of at least √A unique values,
				// all of the Merge steps must be replaced by a different merge algorithm (check the end of the Merge function above)
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
					
					// check A (from start to mid) for the number of unique values we need to fill an internal buffer
					// these values will be pulled out to the start of A
					last = start; count = 1;
					for (index = start + 1; index < mid; index++) {
						if (compare(array[index - 1], array[index])) {
							last = index;
							if (++count >= find) break;
						}
					}
					index = last;
					
					if (count >= buffer_size) {
						// keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer
						pull[pull_index].range = Range(start, end);
						pull[pull_index].count = count;
						pull[pull_index].from = index;
						pull[pull_index].to = start;
						pull_index = 1;
						
						if (count == buffer_size + buffer_size) {
							// we were able to find a single contiguous section containing 2√A unique values,
							// so this section can be used to contain both of the internal buffers we'll need
							buffer1 = Range(start, start + buffer_size);
							buffer2 = Range(start + buffer_size, start + count);
							break;
						} else if (find == buffer_size + buffer_size) {
							buffer1 = Range(start, start + count);
							
							// we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values,
							// so we still need to find a second separate buffer of at least √A unique values
							find = buffer_size;
						} else {
							// we found a second buffer in an 'A' area containing √A unique values, so we're done!
							buffer2 = Range(start, start + count);
							break;
						}
					} else if (pull_index == 0 && count > buffer1.length()) {
						// keep track of the largest buffer we were able to find
						buffer1 = Range(start, start + count);
						
						pull[pull_index].range = Range(start, end);
						pull[pull_index].count = count;
						pull[pull_index].from = index;
						pull[pull_index].to = start;
					}
					
					// check B (from mid to end) for the number of unique values we need to fill an internal buffer
					// these values will be pulled out to the end of B
					last = end - 1; count = 1;
					for (index = end - 2; index >= mid; index--) {
						if (compare(array[index], array[index + 1])) {
							last = index;
							if (++count >= find) break;
						}
					}
					index = last;
					
					if (count >= buffer_size) {
						// keep track of the range within the array where we'll need to "pull out" these values to create the internal buffer
						pull[pull_index].range = Range(start, end);
						pull[pull_index].count = count;
						pull[pull_index].from = index;
						pull[pull_index].to = end;
						pull_index = 1;
						
						if (count == buffer_size + buffer_size) {
							// we were able to find a single contiguous section containing 2√A unique values,
							// so this section can be used to contain both of the internal buffers we'll need
							buffer1 = Range(end - count, end - buffer_size);
							buffer2 = Range(end - buffer_size, end);
							break;
						} else if (find == buffer_size + buffer_size) {
							buffer1 = Range(end - count, end);
							
							// we found a buffer that contains at least √A unique values, but did not contain the full 2√A unique values,
							// so we still need to find a second separate buffer of at least √A unique values
							find = buffer_size;
						} else {
							// we found a second buffer in an 'B' area containing √A unique values, so we're done!
							buffer2 = Range(end - count, end);
							
							// buffer2 will be pulled out from a 'B' area, so if the first buffer was pulled out from the corresponding 'A' area,
							// we need to adjust the end point so it knows to stop redistrubing its values before reaching buffer2
							if (pull[0].range.start == start) pull[0].range.end -= pull[1].count;
							
							break;
						}
					} else if (pull_index == 0 && count > buffer1.length()) {
						// keep track of the largest buffer we were able to find
						buffer1 = Range(end - count, end);
						
						pull[pull_index].range = Range(start, end);
						pull[pull_index].count = count;
						pull[pull_index].from = index;
						pull[pull_index].to = end;
					}
				}
				
				// pull out the two ranges so we can use them as internal buffers
				for (pull_index = 0; pull_index < 2; pull_index++) {
					long length = pull[pull_index].count; count = 0;
					
					if (pull[pull_index].to < pull[pull_index].from) {
						// we're pulling the values out to the left, which means the start of an A area
						for (index = pull[pull_index].from; count < length; index--) {
							if (index == pull[pull_index].to || compare(array[index - 1], array[index])) {
								Rotate(array, -count, Range(index + 1, pull[pull_index].from + 1), cache, cache_size);
								pull[pull_index].from = index + count; count++;
							}
						}
					} else if (pull[pull_index].to > pull[pull_index].from) {
						// we're pulling values out to the right, which means the end of a B area
						for (index = pull[pull_index].from; count < length; index++) {
							if (index == pull[pull_index].to - 1 || compare(array[index], array[index + 1])) {
								Rotate(array, count, Range(pull[pull_index].from, index), cache, cache_size);
								pull[pull_index].from = index - count; count++;
							}
						}
					}
				}
				
				// adjust block_size and buffer_size based on the values we were able to pull out
				buffer_size = buffer1.length();
				block_size = (decimal_step + 1)/buffer_size + 1;
				
				// the first buffer NEEDS to be large enough to tag each of the evenly sized A blocks,
				// so this was originally here to test the math for adjusting block_size above
				//assert((decimal_step + 1)/block_size <= buffer_size);
				
				// now that the two internal buffers have been created, it's time to merge each A+B combination at this level of the merge sort!
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
					
					// calculate the ranges for A and B, and make sure to remove any portions that are being used by the internal buffers
					Range A = Range(start, mid), B = Range(mid, end);
					
					for (pull_index = 0; pull_index < 2; pull_index++) {
						if (start == pull[pull_index].range.start) {
							if (pull[pull_index].from > pull[pull_index].to)
								A.start += pull[pull_index].count;
							else if (pull[pull_index].from < pull[pull_index].to)
								B.end -= pull[pull_index].count;
						}
					}
					
					if (compare(array[B.end - 1], array[A.start])) {
						// the two ranges are in reverse order, so a simple rotation should fix it
						Rotate(array, A.end - A.start, Range(A.start, B.end), cache, cache_size);
					} else if (compare(array[A.end], array[A.end - 1])) {
						// these two ranges weren't already in order, so we'll need to merge them!
						
						// break the remainder of A into blocks. firstA is the uneven-sized first A block
						Range blockA = Range(A.start, A.end);
						Range firstA = Range(A.start, A.start + blockA.length() % block_size);
						
						// swap the second value of each A block with the value in buffer1
						for (index = 0, indexA = firstA.end + 1; indexA < blockA.end; index++, indexA += block_size) 
							std::swap(array[buffer1.start + index], array[indexA]);
						
						// start rolling the A blocks through the B blocks!
						// whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well
						Range lastA = firstA;
						Range lastB = Range(0, 0);
						Range blockB = Range(B.start, B.start + std::min(block_size, B.length()));
						blockA.start += firstA.length();
						
						long minA = blockA.start, indexA = 0;
						__typeof__(*array) min_value = array[minA];
						
						// if the first unevenly sized A block fits into the cache, copy it there for when we go to Merge it
						// otherwise, if the second buffer is available, block swap the contents into that
						if (lastA.length() <= cache_size)
							std::copy(&array[lastA.start], &array[lastA.end], &cache[0]);
						else if (buffer2.length() > 0)
							BlockSwap(array, lastA.start, buffer2.start, lastA.length());
						
						while (true) {
							// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block,
							// then drop that minimum A block behind. or if there are no B blocks left then keep dropping the remaining A blocks.
							if ((lastB.length() > 0 && !compare(array[lastB.end - 1], min_value)) || blockB.length() == 0) {
								// figure out where to split the previous B block, and rotate it at the split
								long B_split = BinaryFirst(array, min_value, lastB, compare);
								long B_remaining = lastB.end - B_split;
								
								// swap the minimum A block to the beginning of the rolling A blocks
								BlockSwap(array, blockA.start, minA, block_size);
								
								// we need to swap the second item of the previous A block back with its original value, which is stored in buffer1
								// since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized
								std::swap(array[blockA.start + 1], array[buffer1.start + indexA++]);
								
								// locally merge the previous A block with the B values that follow it, using the buffer as swap space
								Merge(array, buffer2, lastA, Range(lastA.end, B_split), compare, cache, cache_size);
								
								if (buffer2.length() > 0 || block_size <= cache_size) {
									// copy the previous A block into the cache or buffer2, since that's where we need it to be when we go to merge it anyway
									if (block_size <= cache_size)
										std::copy(&array[blockA.start], &array[blockA.start + block_size], cache);
									else
										BlockSwap(array, blockA.start, buffer2.start, block_size);
									
									// this is equivalent to rotating, but faster
									// the area normally taken up by the A block is either the contents of buffer2, or data we don't need anymore since we memcopied it
									// either way, we don't need to retain the order of those items, so instead of rotating we can just block swap B to where it belongs
									BlockSwap(array, B_split, blockA.start + block_size - B_remaining, B_remaining);
								} else {
									// we are unable to use the 'buffer2' trick to speed up the rotation operation since buffer2 doesn't exist, so perform a normal rotation
									Rotate(array, blockA.start - B_split, Range(B_split, blockA.start + block_size), cache, cache_size);
								}
								
								// now we need to update the ranges and stuff
								lastA = Range(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
								lastB = Range(lastA.end, lastA.end + B_remaining);
								
								blockA.start += block_size;
								if (blockA.length() == 0)
									break;
								
								// search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!)
								minA = blockA.start + 1;
								for (long findA = minA + block_size; findA < blockA.end; findA += block_size)
									if (compare(array[findA], array[minA]))
										minA = findA;
								minA = minA - 1; // decrement once to get back to the start of that A block
								min_value = array[minA];
								
							} else if (blockB.length() < block_size) {
								// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
								// this needs to disable the cache if it determines that the contents of the previous A block are in it
								if (buffer2.length() > 0 || block_size <= cache_size)
									Rotate(array, -blockB.length(), Range(blockA.start, blockB.end), cache, 0); // cache disabled
								else
									Rotate(array, -blockB.length(), Range(blockA.start, blockB.end), cache, cache_size); // cache enabled
								
								lastB = Range(blockA.start, blockA.start + blockB.length());
								blockA.start += blockB.length();
								blockA.end += blockB.length();
								minA += blockB.length();
								blockB.end = blockB.start;
							} else {
								// roll the leftmost A block to the end by swapping it with the next B block
								BlockSwap(array, blockA.start, blockB.start, block_size);
								lastB = Range(blockA.start, blockA.start + block_size);
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
						
						// merge the last A block with the remaining B blocks
						Merge(array, buffer2, lastA, Range(lastA.end, B.end), compare, cache, cache_size);
					}
				}
				
				// when we're finished with this step we should have the one or two internal buffers left over, where the second buffer is all jumbled up
				// insertion sort the second buffer, then redistribute the buffers back into the array using the opposite process used for creating the buffer
				InsertionSort(array, buffer2, compare);
				
				for (pull_index = 0; pull_index < 2; pull_index++) {
					if (pull[pull_index].from > pull[pull_index].to) {
						// the values were pulled out to the left, so redistribute them back to the right
						Range buffer = Range(pull[pull_index].range.start, pull[pull_index].range.start + pull[pull_index].count);
						for (index = buffer.end; buffer.length() > 0; index++) {
							if (index == pull[pull_index].range.end || !compare(array[index], array[buffer.start])) {
								long amount = index - buffer.end;
								Rotate(array, -amount, Range(buffer.start, index), cache, cache_size);
								buffer.start += (amount + 1);
								buffer.end += amount;
								index--;
							}
						}
					} else if (pull[pull_index].from < pull[pull_index].to) {
						// the values were pulled out to the right, so redistribute them back to the left
						Range buffer = Range(pull[pull_index].range.end - pull[pull_index].count, pull[pull_index].range.end);
						for (index = buffer.start; buffer.length() > 0; index--) {
							if (index == pull[pull_index].range.start || !compare(array[buffer.end - 1], array[index - 1])) {
								long amount = buffer.start - index;
								Rotate(array, amount, Range(index, buffer.end), cache, cache_size);
								buffer.start -= amount;
								buffer.end -= (amount + 1);
								index++;
							}
						}
					}
				}
			}
			
			// double the size of each A and B area that will be merged in the next level
			decimal_step += decimal_step;
			fractional_step += fractional_step;
			if (fractional_step >= fractional_base) {
				fractional_step -= fractional_base;
				decimal_step++;
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


using namespace std;

// make sure the items within the given range are in a stable order
template <typename Comparison>
void Verify(const Test array[], const Range range, const Comparison compare, const string msg) {
	for (long index = range.start + 1; index < range.end; index++) {
		// if it's in ascending order then we're good
		// if both values are equal, we need to make sure the index values are ascending
		if (!(compare(array[index - 1], array[index]) ||
			  (!compare(array[index], array[index - 1]) && array[index].index > array[index - 1].index))) {
			
			for (long index2 = range.start; index2 < range.end; index2++)
				cout << array[index2].value << " (" << array[index2].index << ") ";
			
			cout << endl << "failed with message: " << msg << endl;
			assert(false);
		}
	}
}

namespace Testing {
	long Pathological(long index, long total) {
		if (index == 0) return 10;
		else if (index < total/2) return 11;
		else if (index == total - 1) return 10;
		return 9;
	}
	
	// purely random data is one of the few cases where it is slower than stable_sort(),
	// although it does end up only running at about 86% as fast in that situation
	long Random(long index, long total) {
		return rand();
	}
	
	// random distribution of few values was a problem with the last version, but it's better now
	// although the algorithm in the Merge function still isn't the one the paper suggests using!
	long RandomFew(long index, long total) {
		return rand() * 100.0/RAND_MAX;
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
		Testing::RandomFew,
		Testing::MostlyDescending,
		Testing::MostlyAscending,
		Testing::Ascending,
		Testing::Descending,
		Testing::Equal,
		Testing::Jittered,
		Testing::MostlyEqual
	};
	
	// initialize the random-number generator
	srand(time(NULL));
	//srand(10141985); // in case you want the same random numbers
	
	cout << "running test cases... " << flush;
	long total = max_size;
	array1.resize(total);
	array2.resize(total);
	for (int test_case = 0; test_case < sizeof(test_cases)/sizeof(test_cases[0]); test_case++) {
		for (long index = 0; index < total; index++) {
			Test item;
			
			item.value = test_cases[test_case](index, total);
			item.index = index;
			
			array1[index] = array2[index] = item;
		}
		
		Wiki::Sort(array1.begin(), array1.end(), compare);
		
		stable_sort(array2.begin(), array2.end(), compare);
		
		Verify(&array1[0], Range(0, total), compare, "test case failed");
		for (long index = 0; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
	}
	cout << "passed!" << endl;
	
	double total_time = Seconds();
	double total_time1 = 0, total_time2 = 0;
	
	for (total = 0; total <= max_size; total += 2048 * 16) {
		array1.resize(total);
		array2.resize(total);
		
		for (long index = 0; index < total; index++) {
			Test item;
			
			//Testing::Pathological,
			//Testing::Random,
			//Testing::RandomFew,
			//Testing::MostlyDescending,
			//Testing::MostlyAscending,
			//Testing::Ascending,
			//Testing::Descending,
			//Testing::Equal,
			//Testing::Jittered,
			//Testing::MostlyEqual
			
			item.value = Testing::Random(index, total);
			item.index = index;
			
			array1[index] = array2[index] = item;
		}
		
		double time1 = Seconds();
		Wiki::Sort(array1.begin(), array1.end(), compare);
		time1 = Seconds() - time1;
		total_time1 += time1;
		
		double time2 = Seconds();
		//__inplace_stable_sort(array2.begin(), array2.end(), compare);
		stable_sort(array2.begin(), array2.end(), compare);
		time2 = Seconds() - time2;
		total_time2 += time2;
		
		cout << "[" << total << "] WikiSort: " << time1 << " seconds, stable_sort: " << time2 << " seconds (" << time2/time1 * 100.0 << "%)" << endl;
		
		// make sure the arrays are sorted correctly, and that the results were stable
		cout << "verifying... " << flush;
		
		Verify(&array1[0], Range(0, total), compare, "testing the final array");
		for (long index = 0; index < total; index++)
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		
		cout << "correct!" << endl;
	}
	
	total_time = Seconds() - total_time;
	cout << "Tests completed in " << total_time << " seconds" << endl;
	cout << "WikiSort: " << total_time1 << " seconds, stable_sort: " << total_time2 << " seconds (" << total_time2/total_time1 * 100.0 << "%)" << endl;
	
	return 0;
}
