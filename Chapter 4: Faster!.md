Chapter 4: Faster!
======================

Alright, so you've read the documentation, you've pored through the <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">paper by Pok-Son Kim and Arne Kutzner</a>, <i>and</i> you've successfully designed your own in-place merge sort, and... it's 3x slower than a standard merge sort.<br/><br/>

Huh. Now what?<br/><br/>

Well, the first optimization pretty much all sorting algorithms do is apply an insertion sort to the smaller levels of the merge. So instead of merging with sizes of 1, 2, 4, 8, etc., we instead do this:

    for each 16 items in the array
        insertion sort them
    
    for each size 16, 32, 64, 128, ..., count
       and so forth

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

And since there's a good chance the data is already somewhat in order, let's skip merging any sections that are already sorted:

    for each size 16, 32, 64, 128, ..., count
        for each chunk of the array of that size
            get the ranges for A and B
            if (A[last] <= B[first])
                the data was already in order, so we're done!
            else
                merge A and B

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

This should get the performance up to about 75% of a standard merge sort. This is already pretty darn good, but let's take it farther!<br/><br/>

Once I'm done eating, I mean. To be continued…
