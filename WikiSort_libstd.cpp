// modified version of WikiSort, to potentially replace __inplace_stable_sort

#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <assert.h>

namespace wiki {
	
	/**
	 *  @if maint
	 *  This is a helper function for the merge routines.
	 *  @endif
	 */
	template<typename _RandomAccessIterator, typename _Compare>
	inline void
	__merge_with_internal_buffer(_RandomAccessIterator __first, _RandomAccessIterator __middle,
		  _RandomAccessIterator __last, _RandomAccessIterator __buffer, _Compare __comp)
	{
		if (__middle - __first <= 0 || __last - __middle <= 0)
			return;
		
		if (!__comp(*__middle, *(__middle - 1)))
			return;
		
		_RandomAccessIterator __A = __first;
		_RandomAccessIterator __B = __middle;
		_RandomAccessIterator __buf = __buffer;
		_RandomAccessIterator __buf_end = __buffer + (__middle - __first);
		
		std::swap_ranges(__buf, __buf_end, __A);
		
		while (__buf < __buf_end && __B < __last)
		{
			if (!__comp(*__B, *__buf))
				std::swap(*__A++, *__buf++);
			else
				std::swap(*__A++, *__B++);
		}
		
		std::swap_ranges(__buf, __buf_end, __A);
	}
	
	/**
	 *  @if maint
	 *  This is a helper function for the stable sorting routines.
	 *  @endif
	 */
	template <typename _RandomAccessIterator, typename _Compare>
	inline void
	__inplace_stable_sort(_RandomAccessIterator __first,
						  _RandomAccessIterator __last, _Compare __comp)
	{
		const long __size = __last - __first;
		
		// reverse any descending ranges in the array, as that will allow them to sort faster
		_RandomAccessIterator __start, __middle, __end;
		__start = __end = __first;
		for (__middle = __first + 1; __middle < __last; std::advance(__middle, 1))
		{
			if (__comp(*__middle, *(__middle - 1)))
				std::advance(__end, 1);
			else
			{
				std::reverse(__start, __end);
				__start = __end = __middle;
			}
		}
		std::reverse(__start, __end);
		
		if (__last - __first <= 32)
		{
			// insertion sort the array, as there are 32 or fewer items
			std::__insertion_sort(__first, __last, __comp);
			return;
		}
		
		// first insertion sort everything the lowest level
		for (_RandomAccessIterator __merge_index = __end = __first; __merge_index < __last; std::advance(__merge_index, 16))
		{
			_RandomAccessIterator __start = __end;
			__end = std::min(__merge_index + 16, __last);
			std::__insertion_sort(__start, __end, __comp);
		}
		
		// then merge sort the higher levels, which are of sizes 32, 64, 128, etc.
		for (long __merge_size = 16; __merge_size < __size; __merge_size += __merge_size)
		{
			long __block_size = std::max((long)sqrt(__merge_size), (long)3);
			long __buffer_size = __merge_size/__block_size;
			
			// as an optimization, we really only need to pull out an internal buffer once for each level of merges
			// after that we can reuse the same buffer over and over, then redistribute it when we're finished with this level
			_RandomAccessIterator __level1 = __first;
			long __level1_length = 0;
			_RandomAccessIterator __level2, __levelA, __levelB;
			long __level2_length, __levelA_length, __levelB_length;
			
			__end = __first;
			for (_RandomAccessIterator __merge_index = __first; __merge_index < __last - __merge_size; std::advance(__merge_index, __merge_size + __merge_size))
			{
				__start = __end;
				__middle = (__merge_index + __merge_size);
				__end = std::min(__merge_index + __merge_size + __merge_size, __last);
				
				if (__comp(*(__end - 1), *__start))
				{
					// the two ranges are in reverse order, so a simple rotation should fix it
					std::rotate(__start, __middle, __end);
				}
				else if (__comp(*__middle, *(__middle - 1)))
				{
					// these two ranges weren't already in order, so we'll need to merge them!
					_RandomAccessIterator __A = __start;
					long __A_length = (__middle - __start);
					
					_RandomAccessIterator __B = __middle;
					long __B_length = (__end - __middle);
					
					// try to fill up two buffers with unique values in ascending order
					_RandomAccessIterator __bufferA, __bufferB, __buffer1, __buffer2, __blockA, __blockB, __firstA, __lastA, __lastB;
					long __bufferA_length, __bufferB_length, __buffer1_length, __buffer2_length, __blockA_length, __blockB_length, __firstA_length, __lastA_length, __lastB_length;
					
					if (__level1_length > 0)
					{
						// reuse the buffers we found in a previous iteration
						__bufferA = __A; __bufferA_length = 0;
						__bufferB = __B + __B_length; __bufferB_length = 0;
						__buffer1 = __level1; __buffer1_length = __level1_length;
						__buffer2 = __level2; __buffer2_length = __level2_length;
					}
					else
					{
						// the first item is always going to be the first unique value, so let's start searching at the next index
						__buffer1_length = 1;
						for (__buffer1 = __A + 1; __buffer1 < __A + __A_length; std::advance(__buffer1, 1))
							if (__comp(*(__buffer1 - 1), *__buffer1) || __comp(*__buffer1, *(__buffer1 - 1)))
								if (++__buffer1_length == __buffer_size)
									break;
						
						// the first item of the second buffer isn't guaranteed to be the first unique value, so we need to find the first unique item too
						__buffer2_length = 0;
						for (__buffer2 = __buffer1 + 1; __buffer2 < __A + __A_length; std::advance(__buffer2, 1))
							if (__comp(*(__buffer2 - 1), *__buffer2) || __comp(*__buffer2, *(__buffer2 - 1)))
								if (++__buffer2_length == __buffer_size)
									break;
						
						if (__buffer2_length == __buffer_size)
						{
							// we found enough values for both buffers in A
							__bufferA = __buffer2; __bufferA_length = __buffer_size * 2;
							__bufferB = __B + __B_length; __bufferB_length = 0;
							__buffer1 = __A; __buffer1_length = __buffer_size;
							__buffer2 = __A + __buffer_size; __buffer2_length = __buffer_size;
						}
						else if (__buffer1_length == __buffer_size)
						{
							// we found enough values for one buffer in A, so we'll need to find one buffer in B
							__bufferA = __buffer1; __bufferA_length = __buffer_size;
							__buffer1 = __A; __buffer1_length = __buffer_size;
							
							// like before, the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index
							__buffer2_length = 1;
							for (__buffer2 = __B + __B_length - 2; __buffer2 >= __B; std::advance(__buffer2, -1))
								if (__comp(*__buffer2, *(__buffer2 + 1)) || __comp(*(__buffer2 + 1), *__buffer2))
									if (++__buffer2_length == __buffer_size)
										break;
							
							if (__buffer2_length == __buffer_size) {
								__bufferB = __buffer2; __bufferB_length = __buffer_size;
								__buffer2 = __B + __B_length - __buffer_size; __buffer2_length = __buffer_size;
							}
							else
								__buffer1_length = 0; // failure
						}
						else
						{
							// we were unable to find a single buffer in A, so we'll need to find two buffers in B
							__buffer1_length = 1;
							for (__buffer1 = __B + __B_length - 2; __buffer1 >= __B; std::advance(__buffer1, -1))
								if (__comp(*__buffer1, *(__buffer1 + 1)) || __comp(*(__buffer1 + 1), *(__buffer1)))
									if (++__buffer1_length == __buffer_size) break;
							
							__buffer2_length = 0;
							for (__buffer2 = __buffer1 - 1; __buffer2 >= __B; std::advance(__buffer2, -1))
								if (__comp(*(__buffer2), *(__buffer2 + 1)) || __comp(*(__buffer2 + 1), *(__buffer2)))
									if (++__buffer2_length == __buffer_size) break;
							
							if (__buffer2_length == __buffer_size) {
								__bufferA = __A; __bufferA_length = 0;
								__bufferB = __buffer2; __bufferB_length = __buffer_size * 2;
								__buffer1 = __B + __B_length - __buffer_size; __buffer1_length = __buffer_size;
								__buffer2 = __buffer1 - __buffer_size; __buffer2_length = __buffer_size;
							}
							else
								__buffer1_length = 0; // failure
						}
						
						if (__buffer1_length < __buffer_size)
						{
							// we failed to fill both buffers with unique values, which implies we're merging two subarrays with a lot of the same values repeated
							// we can use this knowledge to write a merge operation that is optimized for arrays of repeating values
							while (__A_length > 0 && __B_length > 0)
							{
								// find the first place in B where the first item in A needs to be inserted
								_RandomAccessIterator __mid = std::lower_bound(__B, __B + __B_length, *__A, __comp);
								
								// rotate A into place
								std::rotate(__A, __A + __A_length, __mid);
								
								// calculate the new A and B ranges
								__B_length = __B + __B_length - __mid; __B = __mid;
								__A = std::upper_bound(__A, __A + __A_length, *(__mid - __A_length), __comp);
								__A_length = __B - __A;
							}
							
							continue;
						}
						
						// move the unique values to the start of A if needed
						long __count = 0;
						for (_RandomAccessIterator __index = __bufferA; __count < __bufferA_length; std::advance(__index, -1))
						{
							if (__index == __A || __comp(*(__index - 1), *__index) || __comp(*__index, *(__index - 1)))
							{
								std::rotate(__index + 1, __bufferA + 1 - __count, __bufferA + 1);
								__bufferA = __index + __count++;
							}
						}
						__bufferA = __A;
						
						// move the unique values to the end of B if needed
						__count = 0;
						for (_RandomAccessIterator __index = __bufferB; __count < __bufferB_length; std::advance(__index, 1))
						{
							if (__index == __B + __B_length - 1 || __comp(*__index, *(__index + 1)) || __comp(*(__index + 1), *__index))
							{
								std::rotate(__bufferB, __bufferB + __count, __index);
								__bufferB = __index - __count++;
							}
						}
						__bufferB = __B + __B_length - __bufferB_length;
						
						// reuse these buffers next time!
						__level1 = __buffer1; __level1_length = __buffer1_length;
						__level2 = __buffer2; __level2_length = __buffer2_length;
						__levelA = __bufferA; __levelA_length = __bufferA_length;
						__levelB = __bufferB; __levelB_length = __bufferB_length;
					}
					
					// break the remainder of A into blocks. firstA is the uneven-sized first A block
					__blockA = __bufferA + __bufferA_length; __blockA_length = __A + __A_length - __blockA;
					__firstA = __bufferA + __bufferA_length; __firstA_length = __blockA_length % __block_size;
					
					// swap the second value of each A block with the value in buffer1
					for (_RandomAccessIterator __index = __buffer1, __indexA = __firstA + __firstA_length + 1; __indexA < __blockA + __blockA_length; std::advance(__indexA, __block_size))
						std::swap(*__index++, *__indexA);
					
					// start rolling the A blocks through the B blocks!
					// whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well
					__lastA = __firstA; __lastA_length = __firstA_length;
					__lastB = __lastA; __lastB_length = 0;
					__blockB = __B; __blockB_length = std::min(__block_size, __B_length - __bufferB_length);
					std::advance(__blockA, __firstA_length);
					__blockA_length -= __firstA_length;
					
					_RandomAccessIterator __minA = __blockA, __indexA = __buffer1;
					
					while (true)
					{
						// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block
						if ((__lastB_length > 0 && !__comp(*(__lastB + __lastB_length - 1), *__minA)) || __blockB_length == 0)
						{
							// figure out where to split the previous B block, and rotate it at the split
							_RandomAccessIterator __B_split = std::lower_bound(__lastB, __lastB + __lastB_length, *__minA, __comp);
							long __B_remaining = __lastB + __lastB_length - __B_split;
							
							// swap the minimum A block to the beginning of the rolling A blocks
							std::swap_ranges(__blockA, __blockA + __block_size, __minA);
							
							// we need to swap the second item of the previous A block back with its original value, which is stored in buffer1
							// since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized
							std::swap(*(__blockA + 1), *__indexA++);
							
							// now we need to split the previous B block at B_split and insert the minimum A block in-between the two parts, using a rotation
							std::rotate(__B_split, __B_split + __B_remaining, __blockA + __block_size);
							
							// locally merge the previous A block with the B values that follow it, using the buffer as swap space
							wiki::__merge_with_internal_buffer(__lastA, __lastA + __lastA_length, __B_split, __buffer2, __comp);
							
							// now we need to update the ranges and stuff
							__lastA = __blockA - __B_remaining; __lastA_length = __block_size;
							__lastB = __lastA + __lastA_length; __lastB_length = __B_remaining;
							std::advance(__blockA, __block_size);
							__blockA_length -= __block_size;
							if (__blockA_length == 0)
								break;
							
							// search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!)
							__minA = __blockA + 1;
							for (_RandomAccessIterator __findA = __minA + __block_size; __findA < __blockA + __blockA_length; std::advance(__findA, __block_size))
								if (__comp(*__findA, *__minA))
									__minA = __findA;
							std::advance(__minA, -1); // decrement once to get back to the start of that A block
						}
						else if (__blockB_length < __block_size)
						{
							// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
							std::rotate(__blockA, __blockB, __blockB + __blockB_length);
							
							__lastB = __blockA;
							__lastB_length = __blockB_length;
							std::advance(__blockA, __blockB_length);
							std::advance(__minA, __blockB_length);
							__blockB_length = 0;
						}
						else
						{
							// roll the leftmost A block to the end by swapping it with the next B block
							std::swap_ranges(__blockA, __blockA + __block_size, __blockB);
							
							__lastB = __blockA; __lastB_length = __block_size;
							if (__minA == __blockA)
								__minA = __blockA + __blockA_length;
							std::advance(__blockA, __block_size);
							std::advance(__blockB, __block_size);
							if (__blockB + __blockB_length > __bufferB)
								__blockB_length = __bufferB - __blockB;
						}
					}
					
					// merge the last A block with the remaining B blocks
					wiki::__merge_with_internal_buffer(__lastA, __lastA + __lastA_length, __B + __B_length - __bufferB_length, __buffer2, __comp);
					
					// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
					// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
					__insertion_sort(__buffer2, __buffer2 + __buffer2_length, __comp);
				}
			}
			
			if (__level1_length > 0)
			{
				// redistribute bufferA back into the array
				_RandomAccessIterator __level_start = __levelA;
				for (_RandomAccessIterator __index = __levelA + __levelA_length; __levelA_length > 0; )
				{
					if (__index == __levelB || !__comp(*__index, *__levelA))
					{
						std::rotate(__levelA, __levelA + __levelA_length, __index);
						__levelA = __index - --__levelA_length;
					}
					else
						std::advance(__index, 1);
				}
				
				// redistribute bufferB back into the array
				for (_RandomAccessIterator __index = __levelB; __levelB_length > 0; )
				{
					if (__index == __level_start || !__comp(*(__levelB + __levelB_length - 1), *(__index - 1)))
					{
						std::rotate(__index, __levelB, __levelB + __levelB_length);
						__levelB = __index;
						__levelB_length--;
					}
					else
						std::advance(__index, -1);
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

bool Test__comp(Test item1, Test item2) { return (item1.value < item2.value); }

#define Seconds()					(clock() * 1.0/CLOCKS_PER_SEC)
#define Var(name, value)			__typeof__(value) name = value

int main() {
	const long max_size = 1500000;
	Var(__comp, &Test__comp);
	std::vector<Test> array1, array2;
	
	// initialize the random-number generator
	srand(/*time(NULL)*/ 10141985);
	
	double total_time = Seconds();
	double total_time1 = 0, total_time2 = 0;
	
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
			
			// purely random data is one of the few cases where it is slower than stable_sort(),
			// although it does end up only running at about 60-65% as fast in that situation
			item.value = rand();
			
			array1[index] = array2[index] = item;
		}
		
		double time1 = Seconds();
		wiki::__inplace_stable_sort(array1.begin(), array1.end(), __comp);
		time1 = Seconds() - time1;
		total_time1 += time1;
		
		double time2 = Seconds();
		std::__inplace_stable_sort(array2.begin(), array2.end(), __comp);
		time2 = Seconds() - time2;
		total_time2 += time2;
		
		std::cout << "[" << total << "] wiki: " << time1 << ", merge: " << time2 << " (" << time2/time1 * 100.0 << "%)" << std::endl;
		
		// make sure the arrays are sorted correctly, and that the results were stable
		std::cout << "verifying... " << std::flush;
		for (long index = 1; index < total; index++) assert(__comp(array1[index - 1], array1[index]) || (!__comp(array1[index], array1[index - 1]) && array1[index].index > array1[index - 1].index));
		
		if (total > 0) assert(!__comp(array1[0], array2[0]) && !__comp(array2[0], array1[0]));
		for (long index = 1; index < total; index++) assert(!__comp(array1[index], array2[index]) && !__comp(array2[index], array1[index]));
		std::cout << "correct!" << std::endl;
	}
	
	total_time = Seconds() - total_time;
	std::cout << "tests completed in " << total_time << " seconds" << std::endl;
	std::cout << "wiki: " << total_time1 << ", merge: " << total_time2 << " (" << total_time2/total_time1 * 100.0 << "%)" << std::endl;
	
	return 0;
}
