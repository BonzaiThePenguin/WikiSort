Chapter 4: Faster!
======================

Alright, so you've read the documentation, you've pored through the <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">paper by Pok-Son Kim and Arne Kutzner</a>, <i>and</i> you've successfully designed your own in-place merge sort, and... it's 3x slower than a standard merge sort. Huh.<br/><br/>

Now what?<br/><br/>

Well, the first optimization pretty much all sorting algorithms do is apply an insertion sort to the smaller levels of the merge. So instead of merging with sizes of 1, 2, 4, 8, etc., we do this:

    for each 16 items in the array, up to power_of_two
        insertion sort them
    
    for each size 16, 32, 64, 128, ..., power_of_two
       and so forth

Keep in mind that those groups of "16" items will need to be <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md">scaled in exactly the same way that we scale the ranges as part of the merge sort</a>. It should actually end up sorting anywhere from 16 to <i>31</i> items at a time.

<br/><br/>
Also, since there's a good chance the data is already somewhat in order, let's skip merging any sections that are already sorted:

    for each size 16, 32, 64, 128, ..., power_of_two
        for each chunk of the array of that size, up to power_of_two
            get the ranges for A and B
    >       if (A[last] <= B[first])
    >           the data was already in order, so no need to do anything!
    >       else
                merge A and B

<br/><br/>
And in the off-chance the blocks were in <i>reverse</i> order, a simple rotation "merges" them just fine:

    for each size 16, 32, 64, 128, ..., power_of_two
        for each chunk of the array of that size, up to power_of_two
            get the ranges for A and B
    >       if (B[last] < A[first])
    >           swap the positions of A and B using a rotation
            else if (A[last] <= B[first])
                the data was already in order, so no need to do anything!
            else
                merge A and B

<br/><br/>
Now that the obvious optimizations are out of the way, let's take a closer look at what the rest of the algorithm is doing:

    for each size 16, 32, 64, 128, ..., power_of_two
        for each chunk of the array of that size, up to power_of_two
            get the ranges for A and B
            if (B[last] < A[first])
                swap the positions of A and B using a rotation
            else if (A[last] <= B[first])
                the data was already in order, so we're done!
            else
                merge A and B (shown below):
                
                calculate block_size, block_count, etc.
                split A and B up into blocks
                pull out unique values to fill two of the blocks (buffers)
                tag each of the A blocks using the first buffer
                roll the A blocks through the B blocks and merge them using the second buffer
                insertion sort the second buffer
                redistribute both buffers back through A and B

<br/><br/>
The first thing that should jump out is that we're calculating block_size and block_count on every run of the inner loop, <i>even though the size never changes</i>. In fact, most of these operations could be pulled out from the inner loop! This is because the in-place merge algorithm was designed to be self-contained, wholly separate from any higher-level logic using it.<br/><br/>

But since our goal is to make a super-fast sort, let's break down the barriers and start <i>merging</i> them together (hahaha). Here are the components you can safely pull out from the inner loop:

    for each size 16, 32, 64, 128, ... power_of_two
    >   calculate block_size, block_count, etc.
    >   pull out unique values to fill two of the blocks (buffers)
        
        for each chunk of the array of that size, up to power_of_two
            get the ranges for A and B
            if (B[last] < A[first])
                swap the positions of A and B using a rotation
            else if (A[last] <= B[first])
                the data was already in order, so we're done!
            else
                merge A and B (shown below):
                
                split A and B up into blocks
                tag each of the A blocks using the first buffer
                roll the A blocks through the B blocks and merge them using the second buffer
        
    >   insertion sort the second buffer
    >   redistribute both buffers back through A and B

<br/><br/>
Next up, we are <i>technically</i> allowed to use extra memory storage and still have O(1) memory, as long as that storage is fixed in size. Giving the algorithm a fixed-size cache can speed up many of the common operations, and more importantly it'd allow us to perform a standard merge in situations where A fits within the cache:

    for each chunk of the array of that size, up to power_of_two
        get the ranges for A and B
        if (B[last] < A[first])
            swap the positions of A and B using a rotation
        else if (A[last] <= B[first])
            the data was already in order, so we're done!
        else
    >       if A fits into the cache
    >           copy A into the cache and merge normally
    >       else
                merge A and B using the second buffer

<br/>
And of course we can go through and write cached versions of the tool functions from <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Tools.md">Chapter 1</a>:

    Rotate(array, range, amount)
    >   if the smaller of the two ranges that wil be reversed below fits into the cache
    >       copy that range into the cache
    >       shift the values in the other range over to its destination
    >       copy the first range from the cache to where it belongs
        else
            Reverse(array, MakeRange(range.start, amount))
            Reverse(array, MakeRange(range.start + amount, range.end))
            Reverse(array, range)

<br/><br/>
This should get the performance up to about <b>75%</b> of a standard merge sort. This is already pretty good, but let's take it farther!<br/><br/>


<b>Profile</b><br/>
Profiling the code allows you to see how long each component of the algorithm takes to perform, so you can have some idea of which areas would benefit the most from optimization. It probably shouldn't be much of a surprise that most of the time is spent on the Merge() operation, so let's look at that:

    Block swap the values in A with those in the buffer
    A_count = 0, B_count = 0, insert = 0
    while (A_count < A.length && B_count < B.length)
        if (buffer[A_count] <= array[B.start + B_count])
            Swap(array[A.start + insert], buffer[A_count])
            A_count = A_count + 1
        else
            Swap(array[A.start + insert], array[B.start + B_count])
            B_count = B_count + 1
        insert = insert + 1
    Block swap the remaining part of the buffer with the remaining part of the array

<br/>
Hm, there's really no reason to check A_count and B_count at the start of each run of the while loop, seeing as how we only update one of them each time:

    Block swap the values in A with those in the buffer
    A_count = 0, B_count = 0, insert = 0
    if (A.length > 0 and B.length > 0)
        while (true)
            if (buffer[A_count] <= array[B.start + B_count])
                Swap(array[A.start + insert], buffer[A_count])
                A_count = A_count + 1
    >           insert = insert + 1
    >           if (A_count >= A.length) break
            else
                Swap(array[A.start + insert], array[B.start + B_count])
                B_count = B_count + 1
    >           insert = insert + 1
    >           if (B_count >= B.length) break
    Block swap the remaining part of the buffer with the remaining part of the array

<br/><br/>
<b>Micro-optimize</b><br/>
If we're writing this in C or C++, would it be faster to use pointer math rather than integer math and array lookups? Switch over to pointers, profile it, and confirm that yes, it's a bit faster! It really seems like something that would have been optimized automatically, but that's why profiling and benchmarking is important.

Other things that help include using arrays rather than std::vectors (the arrays are never resized so it doesn't matter), manually declaring that our functions do not raise exceptions (with the throw() keyword), and writing our own binary search and reverse functions. Things that did <i>not</i> help include marking functions as inline, adding a cached version of BlockSwap, or writing our own Swap function – std::swap was exactly as fast.

(technically the sorter <i>can</i> raise an exception via the comparison function, so if you use exceptions then definitely consider leaving that part out)<br/><br/>

<b>Consider the constraints</b><br/>
Since the contents of one of the buffers are allowed to be rearranged (they will be insertion sorted after the merge step is completed), we could speed up some of the operations by using this buffer when the cache was too small to be of use. For example, let's look at the part of the merge step where we just finished "dropping" the smallest A block behind, and need to rotate it into the previous B block:

    Rotate(array, blockA.start - B_split, MakeRange(B_split, blockA.start + block_size))
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

But as was already stated, the A block had to be swapped over to the buffer for the Merge() call anyway, and the contents of the buffer are allowed to be rearranged. By considering those constraints we were able to replace the costly rotation with a simple block swap, and since the operation is performed so often it results in a nice speed boost!<br/><br/>

<b>Is there more room for improvement?</b><br/>
Absolutely. One notable issue is that even if we set the cache to be as large as the original array, which means it ends up performing a standard merge sort, it only runs 95% as fast as it should. Where is that extra 5% of performance going? Beats me.<br/>

There are also a few areas in the algorithm where the cache goes completely unused, simply because nothing could fit <i>entirely</i> within it. Is caching <i>some</i> of the data better than getting all of the data from one source?<br/>

The original paper suggests counting the number of B blocks that need to be swapped with the rolling A blocks <i>before</i> swapping them, but it didn't seem like it would help much so that optimization was skipped.

===========================

<b>That's all for now!</b> The <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp">provided implementation</a> is literally just a direct implementation of these concepts. Hopefully this information has proven valuable, and you can use your newfound knowledge to add your own optimizations and improvements. That way, everyone gets a better sort!
