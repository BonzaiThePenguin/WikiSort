Chapter 4: Faster!
======================

Alright, so you've read the documentation, you've pored through the <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">paper by Pok-Son Kim and Arne Kutzner</a>, <i>and</i> you've successfully designed your own in-place merge sort, and... it's 3x slower than a standard merge sort. Huh.<br/><br/>

Now what?<br/><br/>

Well, the first optimization pretty much all sorting algorithms do is apply an insertion sort to the smaller levels of the merge. So instead of merging with sizes of 1, 2, 4, 8, etc., we instead do this:

    for each 16 items in the array
        insertion sort them
    
    for each size 16, 32, 64, 128, ..., count
       and so forth

<br/><br/>
Another common problem with merge sort is that sorting data that is currently in <i>reverse order</i> can be noticeably slower than other arrangements. A quick fix for this is to simply reverse any descending ranges within the array:

    range = MakeRange(0, 0)
    for (long index = 1; index < count; index++)
        if (array[index] < array[index - 1])
            range.end = range.end + 1
        else
            Reverse(array, range)
            range = MakeRange(index, index)
    Reverse(array, range);

    for each 16 items in the array
        insertion sort them
    
    for each size 16, 32, 64, 128, ..., count
       and so forth

<br/><br/>
And since there's a good chance the data is already somewhat in order, let's skip merging any sections that are already sorted:

    for each size 16, 32, 64, 128, ..., count
        for each chunk of the array of that size
            get the ranges for A and B
            if (A[last] <= B[first])
                the data was already in order, so we're done!
            else
                merge A and B

<br/><br/>
Now that the obvious optimizations are out of the way, let's take a closer look at what the rest of the algorithm is doing:

    for each size 16, 32, 64, 128, ..., count
        for each chunk of the array of that size
            get the ranges for A and B
            if (A[last] <= B[first])
                the data was already in order, so we're done!
            else
                merge A and B (shown below):
                
                calculate block_size, block_count, etc.
                split A and B up into blocks
                pull out unique values to fill two of the blocks (buffers)
                roll the A blocks through the B blocks and merge them
                insertion sort one of the buffers
                redistribute both buffers back through A and B

<br/><br/>
The first thing that should jump out is that we're calculating block_size and block_count on every run of the inner loop, <i>even though the size never changes</i>. In fact, most of these operations could be pulled out from the inner loop! This is because the in-place merge algorithm was designed to be self-contained, wholly separate from any higher-level logic using it.<br/><br/>

But since our goal is to make a super-fast sort, let's break down the barriers and start <i>merging</i> them together (hahaha). Here are the components you can safely pull out from the inner loop:

    for each size 16, 32, 64, 128, ... count
        calculate block_size, block_count, etc.
        pull out unique values to fill two of the blocks (buffers)
        
        for each chunk of the array of that size
            get the ranges for A and B
            if (A[last] <= B[first])
                the data was already in order, so we're done!
            else
                merge A and B (shown below):
                
                split A and B up into blocks
                roll the A blocks through the B blocks and merge them
        
        insertion sort one of the buffers
        redistribute both buffers back through A and B

<br/><br/>
Next up, we are <i>technically</i> allowed to use extra memory storage and still have O(1) memory, as long as that storage is fixed in size. Giving the algorithm a fixed-size cache can speed up many of the common operations, and more importantly it'd allow us to perform a standard merge in situations where A fits within the cache:

    for each chunk of the array of that size
        get the ranges for A and B
        if (A[last] <= B[first])
            the data was already in order, so we're done!
        else
            if A fits into the cache
                copy A into the cache and merge normally
            else
                split A and B up into blocks
                roll the A blocks through the B blocks and merge them

<br/><br/>
This should get the performance up to about 75% of a standard merge sort. This is already pretty good, but let's take it farther!<br/><br/>


<b>Profile</b><br/>
Profiling the code allows you to see how long each component of the algorithm takes to perform, so you can have some idea of which areas would benefit the most from optimization. It probably shouldn't be much of a surprise that most of the time is spent on the Merge() operation, so let's look at that:

    Copy A and B into a buffer
    A_count = 0, B_count = 0, insert = 0
    while (A_count < A.length && B_count < B.length)
        if (buffer[A_count] <= buffer[B.start + B_count])
            array[A.start + insert] = buffer[A_count]
            A_count = A_count + 1
        else
            array[A.start + insert] = buffer[B.start + B_count]
            B_count = B_count + 1
        insert = insert + 1
    Copy the remaining part of the buffer back into the array

<br/>
Hm, there's really no reason to check A_count and B_count at the start of each run of the while loop, seeing as how we only update one of them each time:

    Copy A and B into a buffer
    A_count = 0, B_count = 0, insert = 0
    if (A.length > 0 and B.length > 0)
        while (true)
            if (buffer[A_count] <= buffer[B.start + B_count])
                array[A.start + insert] = buffer[A_count]
                A_count = A_count + 1
    >           if (A_count >= A.length) break
            else
                array[A.start + insert] = buffer[B.start + B_count]
                B_count = B_count + 1
    >           if (B_count >= B.length) break
            insert = insert + 1
    Copy the remaining part of the buffer back into the array

<br/><br/>
<b>Micro-optimize</b><br/>
Also, if we're writing this in C or C++, would it be faster to use pointer math rather than integer math and array lookups? Switch over to pointers, profile it, and confirm that yes, it's a percentage faster!<br/><br/>

<b>Consider the constraints</b><br/>
Since the contents of one of the buffers are allowed to be rearranged (they will be insertion sorted after the merge step is completed), we could speed up some of the operations by using this <i>internal</i> buffer when the cache was too small to be of use. For example, let's look at the part of the merge step where we just finished "dropping" the smallest A block behind, and need to rotate it into the previous B block:

    Rotate(array, B_remaining, MakeRange(B_split, blockA.start + block_size))
    Merge(array, lastA, MakeRange(lastA.end, B_split), using buffer2 as the internal buffer)

We rotate A into the B block to where it belongs (at index B_split), then merge the previous A block with the B values that follow it. Pretty straightforward. However, rotating is a fairly expensive operation (requiring three reversals), and Merge needs to swap the entire contents of lastA with the contents of buffer2 before merging.

What if we instead swapped lastA into buffer2 beforehand, since that's where it needed to be anyway, then we had to rotate the contents of buffer2 into the previous B block? If we did this, we could take advantage of the fact that the contents of that buffer don't need to stay in order:

    Here's what the rotation normally looks like:
    [ previous B block ] [ A block to rotate ]
    [ previous B bl] <-[ock ][ A block to rotate ]
    [ previous B bl]   [ A block to rotate ][ock ] <-
    [ previous B bl][ A block to rotate ][ock ]
    
    But if we swapped the A block with buffer2 beforehand:
    [ previous B block ] [ the second buffer ]
    
    And we get to this step:
    [ previous B bl]   [ock ][ the second buffer ]
    
    We can skip the costly rotation and just block swap:
    [ previous B bl]   [ock ][ the second buf][fer ]
                         ^                      ^
    [ previous B bl]   [fer ][ the second buf][ock ]
                         ^                      ^
    [ previous B bl][fer ][ the second buf][ock ]

<br/>
Both operations end with the two sides of B in the correct spots, but block swapping causes the contents of buffer2 to be in the wrong order, and the contents of the A block are where buffer2 used to be!

But as was already stated, the A block had to be swapped over to the buffer for the Merge() call anyway, and the contents of the buffer are allowed to be rearranged. By considering those constraints we were able to replace the costly rotation with a simple block swap, and since the operation is performed so often it results in a nice speed boost!

===========================

<b>That's it!</b> The <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp">provided implementation</a> is literally just a direct implementation of these concepts. Hopefully this information has proven valuable, and you can use your newfound knowledge to add your own optimizations and improvements. That way, everyone gets a better sort!
