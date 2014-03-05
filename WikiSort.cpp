/*
 g++ -o WikiSort.x WikiSort.cpp -O3 -Wall -pedantic
 ./WikiSort.x
*/

#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <assert.h>

using namespace std;

#define Seconds()					(clock() * 1.0/CLOCKS_PER_SEC)
#define Var(name, value)			__typeof__(value) name = value


// structure to represent ranges within the array
typedef struct {
	long start;
	long length;
} Range;

Range MakeRange(long start, long length) {
	Range range;
	range.start = start;
	range.length = length;
	return range;
}

Range RangeBetween(long start, long end) {
	return MakeRange(start, end - start);
}

Range ZeroRange() {
	return MakeRange(0, 0);
}


// toolbox functions used by the sorter

// if your language does not support bitwise operations for some reason, you can use (floor(value/2) * 2 == value)
bool IsEven(long value) { return ((value & 0x1) == 0x0); }

// 63 -> 32, 64 -> 64, etc.
// apparently this comes from Hacker's Delight?
long FloorPowerOfTwo (long x) {
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
long BinaryFirst(const vector<T> &array, long index, Range range, Comparison compare) {
	typename vector<T>::const_iterator first = lower_bound(array.begin() + range.start, array.begin() + range.start + range.length, array[index], compare);
	return (first - array.begin());
}

// find the index of the last value within the range that is equal to array[index], plus 1
template <typename T, class Comparison>
long BinaryLast(const vector<T> &array, long index, Range range, Comparison compare) {
	typename vector<T>::const_iterator last = upper_bound(array.begin() + range.start, array.begin() + range.start + range.length, array[index], compare);
	return (last - array.begin());
}

// n^2 sorting algorithm used to sort tiny chunks of the full array
template <typename T, class Comparison>
void InsertionSort(vector<T> &array, Range range, Comparison compare) {
	for (long i = range.start + 1; i < range.start + range.length; i++) {
		T temp = array[i]; long j;
		for (j = i; j > range.start && compare(temp, array[j - 1]); j--)
			array[j] = array[j - 1];
		array[j] = temp;
	}
}

// reverse a range within the array
template <typename T>
void Reverse(vector<T> &array, Range range) {
	reverse(array.begin() + range.start, array.begin() + range.start + range.length);
}

// swap a series of values in the array
template <typename T>
void BlockSwap(vector<T> &array, long start1, long start2, long block_size) {
	for (long index = 0; index < block_size; index++)
		swap(array[start1 + index], array[start2 + index]);
}

// rotate the values in an array ([0 1 2 3] becomes [3 0 1 2] if we rotate by +1)
template <typename T>
void Rotate(vector<T> &array, long amount, Range range) {
	if (range.length <= 0) return;
	
	if (amount < 0) amount = range.length - (-amount) % range.length;
	else amount = amount % range.length;
	
	rotate(array.begin() + range.start, array.begin() + range.start + amount, array.begin() + range.start + range.length);
}

// standard merge operation using an internal buffer
template <typename T, class Comparison>
void WikiMerge(vector<T> &array, Range buffer, Range A, Range B, Comparison compare) {
	if (B.length <= 0 || A.length <= 0) return;
	if (!compare(array[B.start], array[A.start + A.length - 1])) return;
	
	// find the part where B will first be inserted into A, as everything before that point is already sorted
	A = RangeBetween(BinaryLast(array, B.start, A, compare), A.start + A.length);
	
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

// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
template <typename T, class Comparison>
void WikiSort(vector<T> &array, Comparison compare) {
	// reverse any descending ranges in the array, as that will allow them to sort faster
	Range reverse = ZeroRange();
	for (long index = 1; index < (long)array.size(); index++) {
		if (compare(array[index], array[index - 1])) reverse.length++;
		else { Reverse(array, reverse); reverse = MakeRange(index, 0); }
	}
	Reverse(array, reverse);
	
	if (array.size() < 32) {
		// insertion sort the array, as there are fewer than 32 items
		InsertionSort(array, RangeBetween(0, array.size()), compare);
		return;
	}
	
	// calculate how to scale the index value to the range within the array
	const long power_of_two = FloorPowerOfTwo(array.size());
	double scale = array.size()/(double)power_of_two; // 1.0 <= scale < 2.0
	
	for (long counter = 0; counter < power_of_two; counter += 32) {
		// insertion sort from start to mid and mid to end
		long start = counter * scale;
		long mid = (counter + 16) * scale;
		long end = (counter + 32) * scale;
		
		InsertionSort(array, RangeBetween(start, mid), compare);
		InsertionSort(array, RangeBetween(mid, end), compare);
		
		// here's where the fake recursion is handled
		// it's a bottom-up merge sort, but multiplying by scale is more efficient than using min(end, array_count)
		long merge = counter, length = 16;
		for (long iteration = counter/16 + 2; IsEven(iteration); iteration /= 2) {
			start = merge * scale;
			mid = (merge + length) * scale;
			end = (merge + length + length) * scale;
			
			// the merges get twice as large after each iteration, until eventually we merge the entire array
			length += length; merge -= length;
			
			if (compare(array[end - 1], array[start])) {
				// the two ranges are in reverse order, so a simple rotation should fix it
				Rotate(array, mid - start, RangeBetween(start, end));
			} else if (compare(array[mid], array[mid - 1])) {
				// these two ranges weren't already in order, so we'll need to merge them!
				Range A = RangeBetween(start, mid), B = RangeBetween(mid, end);
				
				// try to fill up two buffers with unique values in ascending order
				Range bufferA, bufferB, buffer1, buffer2, blockA, blockB, firstA, lastA, lastB;
				long block_size = max((long)sqrt(A.length), (long)3);
				long buffer_size = A.length/block_size;
				
				// the first item is always going to be the first unique value, so let's start searching at the next index
				buffer1.length = 1;
				for (buffer1.start = A.start + 1; buffer1.start < A.start + A.length; buffer1.start++) {
					if (compare(array[buffer1.start - 1], array[buffer1.start]) || compare(array[buffer1.start], array[buffer1.start - 1])) {
						buffer1.length++;
						if (buffer1.length == buffer_size) break;
					}
				}
				
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
					}
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
					}
				}
				
				if (buffer2.length < buffer_size) {
					// we failed to fill both buffers with unique values, which implies we're merging two subarrays with a lot of the same values repeated
					// we can use this knowledge to write a merge operation that is optimized for arrays of repeating values
					while (A.length > 0 && B.length > 0) {
						// find the first place in B where the first item in A needs to be inserted
						long mid = BinaryFirst(array, A.start, B, compare);
						
						// rotate A into place
						long amount = mid - (A.start + A.length);
						Rotate(array, -amount, RangeBetween(A.start, mid));
						
						// calculate the new A and B ranges
						B = RangeBetween(mid, B.start + B.length);
						A = RangeBetween(BinaryLast(array, A.start + amount, A, compare), B.start);
					}
					
					continue;
				}
				
				// move the unique values to the start of A if needed
				for (long index = bufferA.start, count = 0; count < bufferA.length; index--) {
					if (index == A.start || compare(array[index - 1], array[index]) || compare(array[index], array[index - 1])) {
						Rotate(array, -count, RangeBetween(index + 1, bufferA.start + 1));
						bufferA.start = index + count; count++;
					}
				}
				bufferA.start = A.start;
				
				// move the unique values to the end of B if needed
				for (long index = bufferB.start, count = 0; count < bufferB.length; index++) {
					if (index == B.start + B.length - 1 || compare(array[index], array[index + 1]) || compare(array[index + 1], array[index])) {
						Rotate(array, count, RangeBetween(bufferB.start, index));
						bufferB.start = index - count; count++;
					}
				}
				bufferB.start = B.start + B.length - bufferB.length;
				
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
						Rotate(array, B_remaining, RangeBetween(B_split, blockA.start + block_size));
						
						// locally merge the previous A block with the B values that follow it, using the buffer as swap space
						WikiMerge(array, buffer2, lastA, RangeBetween(lastA.start + lastA.length, B_split), compare);
						
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
						Rotate(array, -blockB.length, RangeBetween(blockA.start, blockB.start + blockB.length));
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
				WikiMerge(array, buffer2, lastA, RangeBetween(lastA.start + lastA.length, B.start + B.length - bufferB.length), compare);
				
				// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
				// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
				InsertionSort(array, buffer2, compare);
				
				// redistribute bufferA back into the array
				for (long index = bufferA.start + bufferA.length; bufferA.length > 0; index++) {
					if (index == bufferB.start || !compare(array[index], array[bufferA.start])) {
						long amount = index - (bufferA.start + bufferA.length);
						Rotate(array, -amount, RangeBetween(bufferA.start, index));
						bufferA.start += amount + 1; bufferA.length--; index--;
					}
				}
				
				// redistribute bufferB back into the array
				for (long index = bufferB.start; bufferB.length > 0; index--) {
					if (index == A.start || !compare(array[bufferB.start + bufferB.length - 1], array[index - 1])) {
						long amount = bufferB.start - index;
						Rotate(array, amount, RangeBetween(index, bufferB.start + bufferB.length));
						bufferB.start -= amount; bufferB.length--; index++;
					}
				}
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

int main() {
	const long max_size = 1500000;
	__typeof__(&TestCompare) compare = TestCompare;
	vector<Test> array1, array2;
	
	// initialize the random-number generator
	srand(/*time(NULL)*/ 10141985);
	
	for (long total = 0; total < max_size; total += 2048 * 16) {
		array1.resize(total);
		array2.resize(total);
		
		for (long index = 0; index < total; index++) {
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
			
			//item.value = rand();
			item.value = 1000 + rand() * 1.0/RAND_MAX * 4;
			
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
		if (total > 0) assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (long index = 1; index < total; index++) {
			assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
			assert(compare(array1[index - 1], array1[index]) || (!compare(array1[index], array1[index - 1]) && array1[index].index > array1[index - 1].index));
		}
		cout << "correct!" << endl;
	}
	
	return 0;
}
