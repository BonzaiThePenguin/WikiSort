// modified version of WikiSort, to potentially replace __inplace_stable_sort

#include <iostream>
#include <algorithm>
#include <vector>
#include <math.h>
#include <assert.h>

// if true, Verify() will be called after each merge step to make sure it worked correctly
#define VERIFY false

namespace wiki {
	/**
	 *  @if maint
	 *  This is a helper function for the merge routines.
	 *  @endif
	 */
	template<typename _RandomAccessIterator, typename _Compare>
	void
	verify(_RandomAccessIterator __first, _RandomAccessIterator __last,
		   _Compare __comp, std::string __msg)
	{
		for (_RandomAccessIterator __index = __first + 1; __index < __last; std::advance(__index, 1))
		{
			if (!(__comp(*(__index - 1), *__index) || (!__comp(*__index, *(__index - 1)) && (*__index).index > (*(__index - 1)).index)))
			{
				for (_RandomAccessIterator __index2 = __first; __index2 < __last; std::advance(__index2, 1)) std::cout << (*__index2).value << " (" << (*__index2).index << ") ";
				std::cout << std::endl << "failed with message: " << __msg << std::endl;
				assert(false);
			}
		}
	}
	
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
	 *  This is a helper function for the merge routines.
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
		
		// floor to the next power of two
		long __x = __size;
		__x = __x | (__x >> 1);
		__x = __x | (__x >> 2);
		__x = __x | (__x >> 4);
		__x = __x | (__x >> 8);
		__x = __x | (__x >> 16);
	#if __LP64__
		__x = __x | (__x >> 32);
	#endif
		const long __power_of_two = __x - (__x >> 1);
		const long __fractional_base = __power_of_two/16;
		long __fractional_step = __size % __fractional_base;
		long __decimal_step = __size/__fractional_base;
		
		// first insertion sort everything the lowest level, which is 16-31 items at a time
		long __decimal = 0, __fractional = 0;
		__end = __first;
		for (_RandomAccessIterator __merge_index = __first; __merge_index < __last; std::advance(__merge_index, 16))
		{
			__start = __end;
			
			__decimal += __decimal_step;
			__fractional += __fractional_step;
			if (__fractional >= __fractional_base)
			{
				__fractional -= __fractional_base;
				__decimal++;
			}
			
			__end = __first + __decimal;
			
			std::__insertion_sort(__start, __end, __comp);
		}
		
		// then merge sort the higher levels, which can be 32-63, 64-127, 128-255, etc.
		for (long __merge_size = 16; __merge_size < __power_of_two; __merge_size += __merge_size) {
			long __block_size = sqrt(__decimal_step);
			long __buffer_size = __decimal_step/__block_size + 1;
			
			// as an optimization, we really only need to pull out an internal buffer once for each level of merges
			// after that we can reuse the same buffer over and over, then redistribute it when we're finished with this level
			_RandomAccessIterator __level1_start, __level1_end, __level2_start, __level2_end;
			_RandomAccessIterator __levelA_start, __levelA_end, __levelB_start, __levelB_end;
			_RandomAccessIterator __index;
			
			__level1_start = __level1_end = __first;
			__decimal = __fractional = 0;
			__end = __first;
			for (long __merge_index = 0; __merge_index < __power_of_two - __merge_size; __merge_index += __merge_size + __merge_size) {
				__start = __end;
				
				__decimal += __decimal_step;
				__fractional += __fractional_step;
				if (__fractional >= __fractional_base)
				{
					__fractional -= __fractional_base;
					__decimal++;
				}
				
				__middle = __first + __decimal;
				
				__decimal += __decimal_step;
				__fractional += __fractional_step;
				if (__fractional >= __fractional_base)
				{
					__fractional -= __fractional_base;
					__decimal++;
				}
				
				__end = __first + __decimal;
				
				if (__comp(*(__end - 1), *__start))
				{
					// the two ranges are in reverse order, so a simple rotation should fix it
					std::rotate(__start, __middle, __end);
					if (VERIFY) wiki::verify(__start, __end, __comp, "reversing order via Rotate()");
				}
				else if (__comp(*__middle, *(__middle - 1)))
				{
					// these two ranges weren't already in order, so we'll need to merge them!
					_RandomAccessIterator __A_start = __start, __A_end = __middle;
					_RandomAccessIterator __B_start = __middle, __B_end = __end;
					
					if (VERIFY) wiki::verify(__A_start, __A_end, __comp, "making sure A is valid");
					if (VERIFY) wiki::verify(__B_start, __B_end, __comp, "making sure B is valid");
					
					// try to fill up two buffers with unique values in ascending order
					_RandomAccessIterator __bufferA_start, __bufferA_end, __bufferB_start, __bufferB_end;
					_RandomAccessIterator __buffer1_start, __buffer1_end, __buffer2_start, __buffer2_end;
					_RandomAccessIterator __blockA_start, __blockA_end, __blockB_start, __blockB_end;
					_RandomAccessIterator __lastA_start, __lastA_end, __lastB_start, __lastB_end;
					_RandomAccessIterator __firstA_start, __firstA_end;
					
					if (__level1_end > __level1_start) {
						// reuse the buffers we found in a previous iteration
						__bufferA_start = __bufferA_end = __A_start;
						__bufferB_start = __bufferB_end = __B_end;
						__buffer1_start = __level1_start; __buffer1_end = __level1_end;
						__buffer2_start = __level2_start; __buffer2_end = __level2_end;
					} else {
						// the first item is always going to be the first unique value, so let's start searching at the next index
						long __count = 1;
						for (__buffer1_start = __A_start + 1; __buffer1_start < __A_end; std::advance(__buffer1_start, 1))
							if (__comp(*(__buffer1_start - 1), *__buffer1_start) || __comp(*__buffer1_start, *(__buffer1_start - 1)))
								if (++__count == __buffer_size)
									break;
						__buffer1_end = __buffer1_start + __count;
						
						// the first item of the second buffer isn't guaranteed to be the first unique value, so we need to find the first unique item too
						__count = 0;
						for (__buffer2_start = __buffer1_start + 1; __buffer2_start < __A_end; std::advance(__buffer2_start, 1))
							if (__comp(*(__buffer2_start - 1), *__buffer2_start) || __comp(*__buffer2_start, *(__buffer2_start - 1)))
								if (++__count == __buffer_size)
									break;
						__buffer2_end = __buffer2_start + __count;
						
						if (__buffer2_end - __buffer2_start == __buffer_size) {
							// we found enough values for both buffers in A
							__bufferA_start = __buffer2_start;
							__bufferA_end = __buffer2_start + __buffer_size * 2;
							
							__bufferB_start = __bufferB_end = __B_end;
							
							__buffer1_start = __A_start;
							__buffer1_end = __A_start + __buffer_size;
							
							__buffer2_start = __A_start + __buffer_size;
							__buffer2_end = __A_start + __buffer_size * 2;
							
						} else if (__buffer1_end - __buffer1_start == __buffer_size) {
							// we found enough values for one buffer in A, so we'll need to find one buffer in B
							__bufferA_start = __buffer1_start;
							__bufferA_end = __buffer1_start + __buffer_size;
							
							__buffer1_start = __A_start;
							__buffer1_end = __A_start + __buffer_size;
							
							// like before, the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index
							__count = 1;
							for (__buffer2_start = __B_end - 2; __buffer2_start >= __B_start; std::advance(__buffer2_start, -1))
								if (__comp(*__buffer2_start, *(__buffer2_start + 1)) || __comp(*(__buffer2_start + 1), *__buffer2_start))
									if (++__count == __buffer_size)
										break;
							__buffer2_end = __buffer2_start + __count;
							
							if (__buffer2_end - __buffer2_start == __buffer_size)
							{
								__bufferB_start = __buffer2_start;
								__bufferB_end = __buffer2_start + __buffer_size;
								__buffer2_start = __B_end - __buffer_size;
								__buffer2_end = __B_end;
							}
							else
								__buffer1_end = __buffer1_start; // failure
							
						} else {
							// we were unable to find a single buffer in A, so we'll need to find two buffers in B
							__count = 1;
							for (__buffer1_start = __B_end - 2; __buffer1_start >= __B_start; std::advance(__buffer1_start, -1))
								if (__comp(*__buffer1_start, *(__buffer1_start + 1)) || __comp(*(__buffer1_start + 1), *__buffer1_start))
									if (++__count == __buffer_size)
										break;
							__buffer1_end = __buffer1_start + __count;
							
							__count = 0;
							for (__buffer2_start = __buffer1_start - 1; __buffer2_start >= __B_start; std::advance(__buffer2_start, -1))
								if (__comp(*__buffer2_start, *(__buffer2_start + 1)) || __comp(*(__buffer2_start + 1), *__buffer2_start))
									if (++__count == __buffer_size)
										break;
							__buffer2_end = __buffer2_start + __count;
							
							if (__buffer2_end - __buffer2_start == __buffer_size)
							{
								__bufferA_start = __bufferA_end = __A_start;
								__bufferB_start = __buffer2_start;
								__bufferB_end = __buffer2_start + __buffer_size * 2;
								__buffer1_start = __B_end - __buffer_size;
								__buffer1_end = __B_end;
								__buffer2_start = __buffer1_start - __buffer_size;
								__buffer2_end = __buffer1_start;
							}
							else
								__buffer1_end = __buffer1_start; // failure
							
						}
						
						if (__buffer1_end - __buffer1_start < __buffer_size) {
							// we failed to fill both buffers with unique values, which implies we're merging two subarrays with a lot of the same values repeated
							// we can use this knowledge to write a merge operation that is optimized for arrays of repeating values
							_RandomAccessIterator __oldA_start = __A_start;
							_RandomAccessIterator __oldB_end = __B_end;
							
							while (__A_end > __A_start && __B_end > __B_start)
							{
								// find the first place in B where the first item in A needs to be inserted
								__middle = std::lower_bound(__B_start, __B_end, *__A_start, __comp);
								
								// rotate A into place
								std::rotate(__A_start, __A_end, __middle);
								
								// calculate the new A and B ranges
								__B_start = __middle;
								__A_start = std::upper_bound(__A_start, __A_end, *(__middle - (__A_end - __A_start)), __comp);
								__A_end = __B_start;
							}
							
							if (VERIFY) wiki::verify(__oldA_start, __oldB_end, __comp, "performing a brute-force in-place merge");
							continue;
						}
						
						// move the unique values to the start of A if needed
						long __length = __bufferA_end - __bufferA_start; __count = 0;
						for (__index = __bufferA_start; __count < __length; std::advance(__index, -1))
						{
							if (__index == __A_start || __comp(*(__index - 1), *__index) || __comp(*__index, *(__index - 1)))
							{
								std::rotate(__index + 1, __bufferA_start + 1 - __count, __bufferA_start + 1);
								__bufferA_start = __index + __count; __count++;
							}
						}
						__bufferA_start = __A_start;
						__bufferA_end = __bufferA_start + __length;
						
						if (VERIFY) {
							wiki::verify(__A_start, __A_start + (__bufferA_end - __bufferA_start), __comp, "testing values pulled out from A");
							wiki::verify(__A_start + (__bufferA_end - __bufferA_start), __A_end, __comp, "testing remainder of A");
						}
						
						// move the unique values to the end of B if needed
						__length = __bufferB_end - __bufferB_start; __count = 0;
						for (__index = __bufferB_start; __count < __length; std::advance(__index, 1))
						{
							if (__index == __B_end - 1 || __comp(*__index, *(__index + 1)) || __comp(*(__index + 1), *__index))
							{
								std::rotate(__bufferB_start, __bufferB_start + __count, __index);
								__bufferB_start = __index - __count; __count++;
							}
						}
						__bufferB_start = __B_end - __length;
						__bufferB_end = __B_end;
						
						if (VERIFY) {
							wiki::verify(__B_end - (__bufferB_end - __bufferB_start), __B_end, __comp, "testing values pulled out from B");
							wiki::verify(__B_start, __B_end - (__bufferB_end - __bufferB_start), __comp, "testing remainder of B");
						}
						
						// reuse these buffers next time!
						__level1_start = __buffer1_start; __level1_end = __buffer1_end;
						__level2_start = __buffer2_start; __level2_end = __buffer2_end;
						__levelA_start = __bufferA_start; __levelA_end = __bufferA_end;
						__levelB_start = __bufferB_start; __levelB_end = __bufferB_end;
					}
					
					// break the remainder of A into blocks. firstA is the uneven-sized first A block
					__blockA_start = __bufferA_end;
					__blockA_end = __A_end;
					__firstA_start = __bufferA_end;
					__firstA_end = __bufferA_end + (__blockA_end - __blockA_start) % __block_size;
					
					// swap the second value of each A block with the value in __buffer1
					_RandomAccessIterator __indexA = __firstA_end + 1;
					for (__index = __buffer1_start; __indexA < __blockA_end; std::advance(__indexA, __block_size))
						std::swap(*__index++, *__indexA);
					
					// start rolling the A blocks through the B blocks!
					// whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well
					__lastA_start = __firstA_start;
					__lastA_end = __firstA_end;
					__lastB_start = __lastB_end = __B_start;
					__blockB_start = __B_start;
					__blockB_end = __B_start + std::min(__block_size, (__B_end - __B_start) - (__bufferB_end - __bufferB_start));
					__blockA_start += __firstA_end - __firstA_start;
					
					_RandomAccessIterator __minA = __blockA_start;
					__indexA = __buffer1_start;
					
					while (true)
					{
						// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block
						if ((__lastB_end > __lastB_start && !__comp(*(__lastB_end - 1), *__minA)) || __blockB_end - __blockB_start == 0)
						{
							// figure out where to split the previous B block, and rotate it at the split
							_RandomAccessIterator __B_split = std::lower_bound(__lastB_start, __lastB_end, *__minA, __comp);
							long __B_remaining = __lastB_end - __B_split;
							
							// swap the minimum A block to the beginning of the rolling A blocks
							std::swap_ranges(__blockA_start, __blockA_start + __block_size, __minA);
							
							// we need to swap the second item of the previous A block back with its original value, which is stored in buffer1
							// since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized
							std::swap(*(__blockA_start + 1), *__indexA++);
							
							// now we need to split the previous B block at B_split and insert the minimum A block in-between the two parts, using a rotation
							std::rotate(__B_split, __B_split + __B_remaining, __blockA_start + __block_size);
							
							// locally merge the previous A block with the B values that follow it, using the buffer as swap space
							wiki::__merge_with_internal_buffer(__lastA_start, __lastA_end, __B_split, __buffer2_start, __comp);
							
							// now we need to update the ranges and stuff
							__lastA_start = __blockA_start - __B_remaining;
							__lastA_end = __blockA_start - __B_remaining + __block_size;
							
							__lastB_start = __lastA_end;
							__lastB_end = __lastA_end + __B_remaining;
							
							std::advance(__blockA_start, __block_size);
							if (__blockA_end - __blockA_start == 0)
								break;
							
							// search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!)
							__minA = __blockA_start + 1;
							for (_RandomAccessIterator __findA = __minA + __block_size; __findA < __blockA_end; std::advance(__findA, __block_size))
								if (__comp(*__findA, *__minA))
									__minA = __findA;
							std::advance(__minA, -1); // decrement once to get back to the start of that A block
						}
						else if (__blockB_end - __blockB_start < __block_size)
						{
							// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
							std::rotate(__blockA_start, __blockB_start, __blockB_end);
							__lastB_start = __blockA_start;
							__lastB_end = __blockA_start + (__blockB_end - __blockB_start);
							std::advance(__blockA_start, (__blockB_end - __blockB_start));
							std::advance(__blockA_end, (__blockB_end - __blockB_start));
							std::advance(__minA, (__blockB_end - __blockB_start));
							__blockB_end = __blockB_start;
						}
						else
						{
							// roll the leftmost A block to the end by swapping it with the next B block
							std::swap_ranges(__blockA_start, __blockA_start + __block_size, __blockB_start);
							
							__lastB_start = __blockA_start;
							__lastB_end = __blockA_start + __block_size;
							if (__minA == __blockA_start)
								__minA = __blockA_end;
							std::advance(__blockA_start, __block_size);
							std::advance(__blockA_end, __block_size);
							std::advance(__blockB_start, __block_size);
							std::advance(__blockB_end, __block_size);
							if (__blockB_end > __bufferB_start)
								__blockB_end = __bufferB_start;
						}
					}
					
					// merge the last A block with the remaining B blocks
					wiki::__merge_with_internal_buffer(__lastA_start, __lastA_end, __B_end - (__bufferB_end - __bufferB_start), __buffer2_start, __comp);
					
					// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
					// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
					__insertion_sort(__buffer2_start, __buffer2_end, __comp);
					
					if (VERIFY) wiki::verify(__A_start + (__bufferA_end - __bufferA_start), __B_end - (__bufferB_end - __bufferB_start), __comp, "making sure the local merges worked");
				}
			}
			
			if (__level1_end > __level1_start)
			{
				// redistribute bufferA back into the array
				_RandomAccessIterator __level_start = __levelA_start;
				for (__index = __levelA_end; __levelA_end > __levelA_start; )
				{
					if (__index == __levelB_start || !__comp(*__index, *__levelA_start))
					{
						long amount = __index - __levelA_end;
						std::rotate(__levelA_start, __levelA_end, __index);
						std::advance(__levelA_start, (amount + 1));
						std::advance(__levelA_end, amount);
					}
					else
						std::advance(__index, 1);
				}
				if (VERIFY) wiki::verify(__level_start, __levelB_start, __comp, "redistributed levelA back into the array");
				
				// redistribute bufferB back into the array
				_RandomAccessIterator __level_end = __levelB_end;
				for (__index = __levelB_start; __levelB_end > __levelB_start; )
				{
					if (__index == __level_start || !__comp(*(__levelB_end - 1), *(__index - 1))) {
						long amount = __levelB_start - __index;
						std::rotate(__index, __levelB_start, __levelB_end);
						std::advance(__levelB_start, -amount);
						std::advance(__levelB_end, -(amount + 1));
					}
					else
						std::advance(__index, -1);
				}
				if (VERIFY) wiki::verify(__level_start, __level_end, __comp, "redistributed levelB back into the array");
			}
			
			__decimal_step += __decimal_step;
			__fractional_step += __fractional_step;
			if (__fractional_step >= __fractional_base)
			{
				__fractional_step -= __fractional_base;
				__decimal_step++;
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

double Seconds() { return clock() * 1.0/CLOCKS_PER_SEC; }

int main() {
	const long max_size = 1500000;
	__typeof__(&TestCompare) compare = &TestCompare;
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
		wiki::__inplace_stable_sort(array1.begin(), array1.end(), compare);
		time1 = Seconds() - time1;
		total_time1 += time1;
		
		double time2 = Seconds();
		std::__inplace_stable_sort(array2.begin(), array2.end(), compare);
		time2 = Seconds() - time2;
		total_time2 += time2;
		
		std::cout << "[" << total << "] wiki: " << time1 << ", merge: " << time2 << " (" << time2/time1 * 100.0 << "%)" << std::endl;
		
		// make sure the arrays are sorted correctly, and that the results were stable
		std::cout << "verifying... " << std::flush;
		wiki::verify(array1.begin(), array1.end(), compare, "testing the final array");
		if (total > 0) assert(!compare(array1[0], array2[0]) && !compare(array2[0], array1[0]));
		for (long index = 1; index < total; index++) assert(!compare(array1[index], array2[index]) && !compare(array2[index], array1[index]));
		std::cout << "correct!" << std::endl;
	}
	
	total_time = Seconds() - total_time;
	std::cout << "tests completed in " << total_time << " seconds" << std::endl;
	std::cout << "wiki: " << total_time1 << ", merge: " << total_time2 << " (" << total_time2/total_time1 * 100.0 << "%)" << std::endl;
	
	return 0;
}
