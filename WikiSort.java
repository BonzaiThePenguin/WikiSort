/***********************************************************
 WikiSort: by Arne Kutzner, Pok-Son Kim, and Mike McFadden
 https://github.com/BonzaiThePenguin/WikiSort
***********************************************************/

import java.util.*;
import java.lang.*;
import java.io.*;

class Test {
	public int value;
	public int index;
}

class TestComparator implements Comparator<Test> {
    public int compare(Test a, Test b) {
    	if (a.value < b.value) return -1;
	if (a.value > b.value) return 1;
	return 0;
    }
}

// structure to represent ranges within the array
class Range {
	public int start;
	public int end;
	
	public Range(int start1, int end1) {
		start = start1;
		end = end1;
	}
	
	public Range() {
		start = 0;
		end = 0;
	}
	
	void set(int start1, int end1) {
		start = start1;
		end = end1;
	}
	
	int length() {
		return end - start;
	}
}

class Wiki {
	// toolbox functions used by the sorter
	
	// 63 -> 32, 64 -> 64, etc.
	// apparently this comes from Hacker's Delight?
	static int FloorPowerOfTwo(int value) {
		int x = value;
		x = x | (x >> 1);
		x = x | (x >> 2);
		x = x | (x >> 4);
		x = x | (x >> 8);
		x = x | (x >> 16);
		return x - (x >> 1);
	}
	
	// find the index of the first value within the range that is equal to array[index]
	static int BinaryFirst(Test array[], Test value, Range range, TestComparator comp) {
		int start = range.start, end = range.end - 1;
		while (start < end) {
			int mid = start + (end - start)/2;
			if (comp.compare(array[mid], value) < 0)
				start = mid + 1;
			else
				end = mid;
		}
		if (start == range.end - 1 && comp.compare(array[start], value) < 0) start++;
		return start;
	}
	
	// find the index of the last value within the range that is equal to array[index], plus 1
	static int BinaryLast(Test array[], Test value, Range range, TestComparator comp) {
		int start = range.start, end = range.end - 1;
		while (start < end) {
			int mid = start + (end - start)/2;
			if (comp.compare(value, array[mid]) >= 0)
				start = mid + 1;
			else
				end = mid;
		}
		if (start == range.end - 1 && comp.compare(value, array[start]) >= 0) start++;
		return start;
	}
	
	// n^2 sorting algorithm used to sort tiny chunks of the full array
	static void InsertionSort(Test array[], Range range, TestComparator comp) {
		for (int i = range.start + 1; i < range.end; i++) {
			Test temp = array[i]; int j;
			for (j = i; j > range.start && comp.compare(temp, array[j - 1]) < 0; j--)
				array[j] = array[j - 1];
			array[j] = temp;
		}
	}
	
	// reverse a range within the array
	static void Reverse(Test array[], Range range) {
		for (int index = range.length()/2 - 1; index >= 0; index--) {
			Test swap = array[range.start + index];
			array[range.start + index] = array[range.end - index - 1];
			array[range.end - index - 1] = swap;
		}
	}
	
	// swap a series of values in the array
	static void BlockSwap(Test array[], int start1, int start2, int block_size) {
		for (int index = 0; index < block_size; index++) {
			Test swap = array[start1 + index];
			array[start1 + index] = array[start2 + index];
			array[start2 + index] = swap;
		}
	}
	
	// rotate the values in an array ([0 1 2 3] becomes [1 2 3 0] if we rotate by 1)
	static void Rotate(Test array[], int amount, Range range) {
		if (range.length() == 0) return;
		
		int split;
		if (amount >= 0)
			split = range.start + amount;
		else
			split = range.end + amount;
		
		Range range1 = new Range(range.start, split);
		Range range2 = new Range(split, range.end);
		
		Reverse(array, range1);
		Reverse(array, range2);
		Reverse(array, range);
	}
	
	// standard merge operation using an internal buffer
	static void Merge(Test array[], Range buffer, Range A, Range B, TestComparator comp) {
		// whenever we find a value to add to the final array, swap it with the value that's already in that spot
		// when this algorithm is finished, 'buffer' will contain its original contents, but in a different order
		int A_count = 0, B_count = 0, insert = 0;
		
		if (B.length() > 0 && A.length() > 0) {
			while (true) {
				if (comp.compare(array[B.start + B_count], array[buffer.start + A_count]) >= 0) {
					Test swap = array[A.start + insert];
					array[A.start + insert] = array[buffer.start + A_count];
					array[buffer.start + A_count] = swap;
					A_count++;
					insert++;
					if (A_count >= A.length()) break;
				} else {
					Test swap = array[A.start + insert];
					array[A.start + insert] = array[B.start + B_count];
					array[B.start + B_count] = swap;
					B_count++;
					insert++;
					if (B_count >= B.length()) break;
				}
			}
		}
		
		// swap the remainder of A into the final array
		BlockSwap(array, buffer.start + A_count, A.start + insert, A.length() - A_count);
	}
	
	// bottom-up merge sort combined with an in-place merge algorithm for O(1) memory use
	static void Sort(Test array[], TestComparator comp) {
		int size = array.length;
		
		// reverse any descending ranges in the array, as that will allow them to sort faster
		Range reverse = new Range(0, 1);
		for (int index = 1; index < size; index++) {
			if (comp.compare(array[index], array[index - 1]) < 0)
				reverse.end++;
			else {
				Reverse(array, reverse);
				reverse.set(index, index + 1);
			}
		}
		Reverse(array, reverse);
		
		// if there are 32 or fewer items, just insertion sort the entire array
		if (size <= 32) {
			InsertionSort(array, new Range(0, size), comp);
			return;
		}
		
		// calculate how to scale the index value to the range within the array
		// (this is essentially fixed-point math, where we manually check for and handle overflow)
		int power_of_two = FloorPowerOfTwo(size);
		int fractional_base = power_of_two/16;
		int fractional_step = size % fractional_base;
		int decimal_step = size/fractional_base;
		
		// first insertion sort everything the lowest level, which is 16-31 items at a time
		int decimal = 0, fractional = 0;
		while (decimal < size) {
			int start = decimal;
			
			decimal += decimal_step;
			fractional += fractional_step;
			if (fractional >= fractional_base) {
				fractional -= fractional_base;
				decimal++;
			}
			
			int end = decimal;
			
			InsertionSort(array, new Range(start, end), comp);
		}
		
		// then merge sort the higher levels, which can be 32-63, 64-127, 128-255, etc.
		for (int merge_size = 16; merge_size < power_of_two; merge_size += merge_size) {
			int block_size = (int)Math.sqrt(decimal_step);
			int buffer_size = decimal_step/block_size + 1;
			
			// as an optimization, we really only need to pull out an internal buffer once for each level of merges
			// after that we can reuse the same buffer over and over, then redistribute it when we're finished with this level
			Range level1 = new Range(), level2 = new Range();
			Range levelA = new Range(), levelB = new Range();
			
			decimal = fractional = 0;
			while (decimal < size) {
				int start = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				int mid = decimal;
				
				decimal += decimal_step;
				fractional += fractional_step;
				if (fractional >= fractional_base) {
					fractional -= fractional_base;
					decimal++;
				}
				
				int end = decimal;
				
				if (comp.compare(array[end - 1], array[start]) < 0) {
					// the two ranges are in reverse order, so a simple rotation should fix it
					Rotate(array, mid - start, new Range(start, end));
					
				} else if (comp.compare(array[mid], array[mid - 1]) < 0) {
					// these two ranges weren't already in order, so we'll need to merge them!
					Range A = new Range(start, mid), B = new Range(mid, end);
					
					Range bufferA = new Range(), bufferB = new Range();
					Range buffer1 = new Range(), buffer2 = new Range();
					Range blockA = new Range(), blockB = new Range();
					Range lastA = new Range(), lastB = new Range();
					Range firstA = new Range();
					
					// try to fill up two buffers with unique values in ascending order
					if (level1.length() > 0) {
						// reuse the buffers we found in a previous iteration
						bufferA.set(A.start, A.start);
						bufferB.set(B.end, B.end);
						buffer1.set(level1.start, level1.end);
						buffer2.set(level2.start, level2.end);
						
					} else {
						// the first item is always going to be the first unique value, so let's start searching at the next index
						int count = 1;
						for (buffer1.start = A.start + 1; buffer1.start < A.end; buffer1.start++)
							if (comp.compare(array[buffer1.start - 1], array[buffer1.start]) != 0)
								if (++count == buffer_size)
									break;
						buffer1.end = buffer1.start + count;
						
						// the first item of the second buffer isn't guaranteed to be the first unique value, so we need to find the first unique item too
						count = 0;
						for (buffer2.start = buffer1.start + 1; buffer2.start < A.end; buffer2.start++)
							if (comp.compare(array[buffer2.start - 1], array[buffer2.start]) != 0)
								if (++count == buffer_size)
									break;
						buffer2.end = buffer2.start + count;
						
						if (buffer2.length() == buffer_size) {
							// we found enough values for both buffers in A
							bufferA.set(buffer2.start, buffer2.start + buffer_size * 2);
							bufferB.set(B.end, B.end);
							buffer1.set(A.start, A.start + buffer_size);
							buffer2.set(A.start + buffer_size, A.start + buffer_size * 2);
							
						} else if (buffer1.length() == buffer_size) {
							// we found enough values for one buffer in A, so we'll need to find one buffer in B
							bufferA.set(buffer1.start, buffer1.start + buffer_size);
							buffer1.set(A.start, A.start + buffer_size);
							
							// like before, the last value is guaranteed to be the first unique value we encounter, so we can start searching at the next index
							count = 1;
							for (buffer2.start = B.end - 2; buffer2.start >= B.start; buffer2.start--)
								if (comp.compare(array[buffer2.start], array[buffer2.start + 1]) != 0)
									if (++count == buffer_size)
										break;
							buffer2.end = buffer2.start + count;
							
							if (buffer2.length() == buffer_size) {
								bufferB.set(buffer2.start, buffer2.start + buffer_size);
								buffer2.set(B.end - buffer_size, B.end);
								
							} else buffer1.end = buffer1.start; // failure
						} else {
							// we were unable to find a single buffer in A, so we'll need to find two buffers in B
							count = 1;
							for (buffer1.start = B.end - 2; buffer1.start >= B.start; buffer1.start--)
								if (comp.compare(array[buffer1.start], array[buffer1.start + 1]) != 0)
									if (++count == buffer_size)
										break;
							buffer1.end = buffer1.start + count;
							
							count = 0;
							for (buffer2.start = buffer1.start - 1; buffer2.start >= B.start; buffer2.start--)
								if (comp.compare(array[buffer2.start], array[buffer2.start + 1]) != 0)
									if (++count == buffer_size)
										break;
							buffer2.end = buffer2.start + count;
							
							if (buffer2.length() == buffer_size) {
								bufferA.set(A.start, A.start);
								bufferB.set(buffer2.start, buffer2.start + buffer_size * 2);
								buffer1.set(B.end - buffer_size, B.end);
								buffer2.set(buffer1.start - buffer_size, buffer1.start);
								
							} else buffer1.end = buffer1.start; // failure
						}
						
						if (buffer1.length() < buffer_size) {
							// we failed to fill both buffers with unique values, which implies we're merging two subarrays with a lot of the same values repeated
							// we can use this knowledge to write a merge operation that is optimized for arrays of repeating values
							while (A.length() > 0 && B.length() > 0) {
								// find the first place in B where the first item in A needs to be inserted
								int split = BinaryFirst(array, array[A.start], B, comp);
								
								// rotate A into place
								int amount = split - A.end;
								Rotate(array, -amount, new Range(A.start, split));
								
								// calculate the new A and B ranges
								B.start = split;
								A.set(BinaryLast(array, array[A.start + amount], A, comp), B.start);
							}
							
							continue;
						}
						
						// move the unique values to the start of A if needed
						int length = bufferA.length();
						count = 0;
						for (int index = bufferA.start; count < length; index--) {
							if (index == A.start || comp.compare(array[index - 1], array[index]) != 0) {
								Rotate(array, -count, new Range(index + 1, bufferA.start + 1));
								bufferA.start = index + count; count++;
							}
						}
						bufferA.set(A.start, A.start + length);
						
						// move the unique values to the end of B if needed
						length = bufferB.length();
						count = 0;
						for (int index = bufferB.start; count < length; index++) {
							if (index == B.end - 1 || comp.compare(array[index], array[index + 1]) != 0) {
								Rotate(array, count, new Range(bufferB.start, index));
								bufferB.start = index - count; count++;
							}
						}
						bufferB.set(B.end - length, B.end);
						
						// reuse these buffers next time!
						level1.set(buffer1.start, buffer1.end);
						level2.set(buffer2.start, buffer2.end);
						levelA.set(bufferA.start, bufferA.end);
						levelB.set(bufferB.start, bufferB.end);
					}
					
					// break the remainder of A into blocks. firstA is the uneven-sized first A block
					blockA.set(bufferA.end, A.end);
					firstA.set(bufferA.end, bufferA.end + blockA.length() % block_size);
					
					// swap the second value of each A block with the value in buffer1
					int index = 0;
					for (int indexA = firstA.end + 1; indexA < blockA.end; indexA += block_size) {
						Test swap = array[buffer1.start + index];
						array[buffer1.start + index] = array[indexA];
						array[indexA] = swap;
						index++;
					}
					
					// start rolling the A blocks through the B blocks!
					// whenever we leave an A block behind, we'll need to merge the previous A block with any B blocks that follow it, so track that information as well
					lastA.set(firstA.start, firstA.end);
					lastB.set(0, 0);
					blockB.set(B.start, B.start + Math.min(block_size, B.length() - bufferB.length()));
					blockA.start += firstA.length();
					
					int minA = blockA.start;
					int indexA = 0;
					Test min_value = array[minA];
					
					BlockSwap(array, lastA.start, buffer2.start, lastA.length());
					
					while (true) {
						// if there's a previous B block and the first value of the minimum A block is <= the last value of the previous B block
						if ((lastB.length() > 0 && comp.compare(array[lastB.end - 1], min_value) >= 0) || blockB.length() == 0) {
							// figure out where to split the previous B block, and rotate it at the split
							int B_split = BinaryFirst(array, min_value, lastB, comp);
							int B_remaining = lastB.end - B_split;
							
							// swap the minimum A block to the beginning of the rolling A blocks
							BlockSwap(array, blockA.start, minA, block_size);
							
							// we need to swap the second item of the previous A block back with its original value, which is stored in buffer1
							// since the firstA block did not have its value swapped out, we need to make sure the previous A block is not unevenly sized
							Test swap = array[blockA.start + 1];
							array[blockA.start + 1] = array[buffer1.start + indexA];
							array[buffer1.start + indexA] = swap;
							indexA++;
							
							// locally merge the previous A block with the B values that follow it, using the buffer as swap space
							Merge(array, buffer2, lastA, new Range(lastA.end, B_split), comp);
							
							// copy the previous A block into the cache or buffer2, since that's where we need it to be when we go to merge it anyway
							BlockSwap(array, blockA.start, buffer2.start, block_size);
							
							// this is equivalent to rotating, but faster
							// the area normally taken up by the A block is either the contents of buffer2, or data we don't need anymore since we memcopied it
							// either way, we don't need to retain the order of those items, so instead of rotating we can just block swap B to where it belongs
							BlockSwap(array, B_split, blockA.start + block_size - B_remaining, B_remaining);
							
							// now we need to update the ranges and stuff
							lastA.set(blockA.start - B_remaining, blockA.start - B_remaining + block_size);
							lastB.set(lastA.end, lastA.end + B_remaining);
							
							blockA.start += block_size;
							if (blockA.length() == 0)
								break;
							
							// search the second value of the remaining A blocks to find the new minimum A block (that's why we wrote unique values to them!)
							minA = blockA.start + 1;
							for (int findA = minA + block_size; findA < blockA.end; findA += block_size)
								if (comp.compare(array[findA], array[minA]) < 0) minA = findA;
							minA = minA - 1; // decrement once to get back to the start of that A block
							min_value = array[minA];
							
						} else if (blockB.length() < block_size) {
							// move the last B block, which is unevenly sized, to before the remaining A blocks, by using a rotation
							Rotate(array, -blockB.length(), new Range(blockA.start, blockB.end));
							lastB.set(blockA.start, blockA.start + blockB.length());
							blockA.start += blockB.length();
							blockA.end += blockB.length();
							minA += blockB.length();
							blockB.end = blockB.start;
						} else {
							// roll the leftmost A block to the end by swapping it with the next B block
							BlockSwap(array, blockA.start, blockB.start, block_size);
							lastB.set(blockA.start, blockA.start + block_size);
							if (minA == blockA.start)
								minA = blockA.end;
							
							blockA.start += block_size;
							blockA.end += block_size;
							blockB.start += block_size;
							blockB.end += block_size;
							
							if (blockB.end > bufferB.start)
								blockB.end = bufferB.start;
						}
					}
					
					// merge the last A block with the remaining B blocks
					Merge(array, buffer2, lastA, new Range(lastA.end, B.end - bufferB.length()), comp);
				}
			}
			
			if (level1.length() > 0) {
				// when we're finished with this step we should have b1 b2 left over, where one of the buffers is all jumbled up
				// insertion sort the jumbled up buffer, then redistribute them back into the array using the opposite process used for creating the buffer
				InsertionSort(array, level2, comp);
				
				// redistribute bufferA back into the array
				int level_start = levelA.start;
				for (int index = levelA.end; levelA.length() > 0; index++) {
					if (index == levelB.start || comp.compare(array[index], array[levelA.start]) >= 0) {
						int amount = index - levelA.end;
						Rotate(array, -amount, new Range(levelA.start, index));
						levelA.start += (amount + 1);
						levelA.end += amount;
						index--;
					}
				}
				
				// redistribute bufferB back into the array
				for (int index = levelB.start; levelB.length() > 0; index--) {
					if (index == level_start || comp.compare(array[levelB.end - 1], array[index - 1]) >= 0) {
						int amount = levelB.start - index;
						Rotate(array, amount, new Range(index, levelB.end));
						levelB.start -= amount;
						levelB.end -= (amount + 1);
						index++;
					}
				}
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

class Main {
	static double Seconds() {
		return System.currentTimeMillis()/1000.0;
	}
	
	// standard merge sort, so we have a baseline for how well the in-place merge works
	static void MergeSortR(Test array[], Range range, TestComparator comp, Test buffer[]) {
		if (range.length() < 32) {
			Wiki.InsertionSort(array, range, comp);
			return;
		}
		
		int mid = range.start + (range.end - range.start)/2;
		Range A = new Range(range.start, mid);
		Range B = new Range(mid, range.end);
		
		MergeSortR(array, A, comp, buffer);
		MergeSortR(array, B, comp, buffer);
		
		// standard merge operation here (only A is copied to the buffer, and only the parts that weren't already where they should be)
		A.set(Wiki.BinaryLast(array, array[B.start], A, comp), A.end);
		java.lang.System.arraycopy(array, A.start, buffer, 0, A.length());
		int A_count = 0, B_count = 0, insert = 0;
		while (A_count < A.length() && B_count < B.length()) {
			if (comp.compare(array[A.end + B_count], buffer[A_count]) >= 0) {
				array[A.start + insert] = buffer[A_count];
				A_count++;
			} else {
				array[A.start + insert] = array[A.end + B_count];
				B_count++;
			}
			insert++;
		}
		
		java.lang.System.arraycopy(buffer, A_count, array, A.start + insert, A.length() - A_count);
	}
	
	static void MergeSort(Test array[], TestComparator comp) {
		MergeSortR(array, new Range(0, array.length), comp, new Test[array.length]);
	}
	
	static void Verify(Test array[], Range range, TestComparator comp, String msg) {
		for (int index = range.start + 1; index < range.end; index++) {
			// if it's in ascending order then we're good
			// if both values are equal, we need to make sure the index values are ascending
			if (!(comp.compare(array[index - 1], array[index]) < 0 ||
				  (comp.compare(array[index], array[index - 1]) == 0 && array[index].index > array[index - 1].index))) {
				
				for (int index2 = range.start; index2 < range.end; index2++)
					System.out.println(array[index2].value + " (" + array[index2].index + ")");
				
				System.out.println("failed with message: " + msg);
				throw new RuntimeException();
			}
		}
	}
	
	public static void main (String[] args) throws java.lang.Exception {
		int max_size = 1500000;
		TestComparator comp = new TestComparator();
		Test[] array1;
		Test[] array2;
		
		// set the seed on the random number generator
		Random rand = new Random(10141985);
		
		int total;
		
		double total_time = Seconds();
		double total_time1 = 0, total_time2 = 0;
		
		for (total = 0; total < max_size; total += 2048 * 16) {
			array1 = new Test[total];
			array2 = new Test[total];
			
			for (int index = 0; index < total; index++) {
				Test item = new Test();
				
				item.value = (int)(rand.nextInt(192585));
				item.index = index;
				
				array1[index] = item;
				array2[index] = item;
			}
			
			double time1 = Seconds();
			Wiki.Sort(array1, comp);
			time1 = Seconds() - time1;
			total_time1 += time1;
			
			double time2 = Seconds();
			MergeSort(array2, comp);
			time2 = Seconds() - time2;
			total_time2 += time2;
			
			System.out.println("[" + total + "] wiki: " + time1 + ", merge: " + time2 + " (" + time2/time1 * 100 + "%)");
			
			// make sure the arrays are sorted correctly, and that the results were stable
			System.out.println("verifying...");
			
			Verify(array1, new Range(0, total), comp, "testing the final array");
			if (total > 0)
				if (comp.compare(array1[0], array2[0]) != 0) throw new Exception();
			for (int index = 1; index < total; index++) {
				if (comp.compare(array1[index], array2[index]) != 0) throw new Exception();
				if (array2[index].index != array1[index].index) throw new Exception();
			}
			
			System.out.println("correct!");
		}
		
		total_time = Seconds() - total_time;
		System.out.println("tests completed in " + total_time + " seconds");
		System.out.println("wiki: " + total_time1 + ", merge: " + total_time2 + " (" + total_time2/total_time1 * 100 + "%)");
	}
}
