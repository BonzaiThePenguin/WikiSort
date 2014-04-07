Chapter 4: Faster!
==================

Alright, so you've read the documentation, you've pored through the [paper by
Pok-Son Kim and Arne Kutzner](http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF],
*and* you've successfully designed your own in-place merge sort, and... it's
3x slower than a standard merge sort. Huh.

* * *

Now what?

Well, the first optimization pretty much all sorting algorithms do is apply
an insertion sort to the smaller levels of the merge. So instead of merging
with sizes of 1, 2, 4, 8, etc., we do this:

    for each 16 items in the array, up to power_of_two
        insertion sort them

    for each size 16, 32, 64, 128, ..., power_of_two
       and so forth

Keep in mind that those groups of "16" items will need to be [scaled in
exactly the same way that we scale the ranges as part of the merge sort]
(https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202.%20Merging.md).
It should actually end up sorting anywhere from 16 to *31* items at a time.

* * *

Also, since there's a good chance the data is already somewhat in order,
let's skip merging any sections that are already sorted:

    for each size 16, 32, 64, 128, ..., power_of_two
        for each chunk of the array of that size, up to power_of_two
            get the ranges for A and B
    >       if (A[last] <= B[first])
    >           the data was already in order, so no need to do anything!
    >       else
                merge A and B

* * *

And in the off-chance the blocks were in *reverse* order, a simple rotation
"merges" them just fine:

    for each size 16, 32, 64, 128, ..., power_of_two
        for each chunk of the array of that size, up to power_of_two
            get the ranges for A and B
    >       if (B[last] < A[first])
    >           swap the positions of A and B using a rotation
            else if (A[last] <= B[first])
                the data was already in order, so no need to do anything!
            else
                merge A and B

* * *

Now that the obvious optimizations are out of the way, let's take a closer
look at what the rest of the algorithm is doing:

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

* * *

The first thing that should jump out is that we're calculating block_size and
block_count on every run of the inner loop, *even though the size never changes*.
In fact, most of these operations could be pulled out from the inner loop!
This is because the in-place merge algorithm was designed to be self-contained,
and we just threw it into a bottom-up merge sort as-is.

* * *

But since our goal is to make a super-fast sort, let's break down the barriers
and start *merging* them together (hahaha). Here are the components you can
safely pull out from the inner loop:

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

* * *

Next up, we are *technically* allowed to use extra memory storage and still
have O(1) memory, as long as that storage is fixed in size. Giving the algorithm
a fixed-size cache can speed up many of the common operations, and more
importantly it'd allow us to perform a standard merge in situations where
A fits within the cache:

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


And of course we can go through and write cached versions of the tool functions
from [Chapter 1]
(https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201.%20Tools.md):

    Rotate(array, range, amount)
    >   if the smaller of the two ranges that will be reversed below fits into the cache
    >       copy that range into the cache
    >       shift the values in the other range over to its destination
    >       copy the first range from the cache to where it belongs
        else
            Reverse(array, MakeRange(range.start, amount))
            Reverse(array, MakeRange(range.start + amount, range.end))
            Reverse(array, range)

* * *

This should get the performance up to about **75%** of a standard merge sort.
This is already pretty good, but let's take it farther!

* * *

**Profile**

Profiling the code allows you to see how long each component of the algorithm
takes to perform, so you can have some idea of which areas would benefit the
most from optimization. It probably shouldn't be much of a surprise that most
of the time is spent on the Merge() operation, so let's look at that:

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

Hm, there's really no reason to check A_count and B_count at the start of each
run of the while loop, seeing as how we only update one of them each time:

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

* * *

**Micro-optimize**

If we're writing this in C or C++, would it be faster to use pointer math
rather than integer math and array lookups? Switch over to pointers, profile
it, and confirm that yes, it's a bit faster! It really seems like something
that would have been optimized automatically, but that's why profiling and
benchmarking is important.

Things that did *not* help include marking functions as inline (the compiler
seems better at figuring it out on its own), adding a cached version of
BlockSwap, or writing our own Swap function – std::swap was exactly as fast.

* * *

**Consider the constraints**

Since the contents of one of the buffers are allowed to be rearranged (they
will be insertion sorted after the merge step is completed), we could speed
up some of the operations by using this buffer when the cache was too small
to be of use. For example, let's look at the part of the merge step where we
just finished "dropping" the smallest A block behind, and need to rotate it
into the previous B block:

    Rotate(array, blockA.start - B_split, MakeRange(B_split, blockA.start + block_size))
    Merge(array, lastA, MakeRange(lastA.end, B_split), using buffer2 as the internal buffer)

We rotate A into the B block to where it belongs (at index B_split), then merge
the previous A block with the B values that follow it. Pretty straightforward.
However, rotating is a fairly expensive operation (requiring three reversals),
and Merge needs to swap the entire contents of lastA with the contents of
buffer2 before merging.

What if we instead swapped lastA into buffer2 beforehand, since that's where
it needed to be anyway, then we had to rotate the contents of buffer2 into
the previous B block? If we did this, we could take advantage of the fact
that the contents of that buffer don't need to stay in order:

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

Both operations end with the two sides of B in the correct spots, but block
swapping causes the contents of buffer2 to be in the wrong order, and the
contents of the A block are where buffer2 used to be!

But as was already stated, the A block had to be swapped over to the buffer
for the Merge() call anyway, and the contents of the buffer are allowed to be
rearranged. By considering those constraints we were able to replace the costly
rotation with a simple block swap, and since the operation is performed so
often it results in a nice speed boost!

* * *

Another useful optimization is merging two A+B pairs at the same time if it
can fit into the cache, then merging the two remaining subarrays back into
the original array. This requires another Merge function:

    MergeInto(from, A, B, into, at_index)
        A_index = A.start, B_index = B.start
        A_last = A.end, B_last = B.end
        insert = at_index
        
        if (B.length > 0 and A.length > 0)
            while (true)
                if (from[B_index] >= from[A_index])
                    into[insert++] = from[A_index++]
                    if (A_index = A_last) break
                else
                    into[insert++] = from[B_index++]
                    if (B_index = B_last) break
        
        // copy the remainder of A and B into the final array

And it uses another branch in the merge sort:

    // if four subarrays fit into the cache, it's faster to merge both pairs of subarrays into the cache,
    // then merge the two merged subarrays from the cache back into the original array
    if ((iterator.length + 1) * 4 < cache_size and size/iterator.length >= 4)
        iterator.begin()
        while (!iterator.finished)
            // handle two A+B pairs at a time!
            Range A1 = iterator.nextRange
            Range B1 = iterator.nextRange
            Range A2 = iterator.nextRange
            Range B2 = iterator.nextRange
            
            // merge A1 and B1 into the cache
            // merge A2 and B2 into the cache
            // merge (A1+B1) and (A2+B2) from the cache into the array

* * *

**That's all for now!** The [provided implementation]
(https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.cpp)
is literally just a direct implementation of these concepts. Hopefully this
information has proven valuable, and you can use your newfound knowledge to add
your own optimizations and improvements. That way, everyone gets a better sort!
