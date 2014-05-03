Chapter 3: In-place
===================

To reiterate, much of the work presented here is based on a paper called
["Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner]
(http://ak.hanyang.ac.kr/papers/tamc2008.pdf) [PDF]. The main differences are
the removal of the movement imitation buffer *s1 t s2*, interlacing the
"block rolling" with the local merges, and adapting it to work efficiently with
a bottom-up merge sort.

* * *

**The basic idea**

The core of efficient in-place merging relies on the fact that merging two
sorted arrays, A and B, is equivalent to breaking A into evenly sized blocks,
inserting them into B, then merging each A block with any values from B that
follow it. So instead of approaching the problem like this:

    [              A                  ][                   B               ]

We think of it like this:

    1. break A into evenly sized blocks
    [ A ][ A ][ A ][ A ][ A ][ A ][ A ][                   B               ]

    2. insert them into B where they belong
       (the rule for inserting is that the first value of each A block must be
        less than or equal to the first value of the next B block)
    [ A ][  B  ][ A ][B][ A ][ A ][   B   ][ A ][  B ][ A ][B][ A ][   B   ]

    3. merge each [A][B] combination
    [ A ][  B  ]  [ A ][B]  [ A ]  [ A ][   B   ]  [ A ][  B ]  [ A ][B]  [ A ][   B   ]

    4. all finished!
    [ A  +  B  ]  [ A + B]  [ A ]  [  A  +   B  ]  [ A  +  B ]  [ A + B]  [ A   +   B  ]
      = [ A  +  B  ][ A + B][ A ][  A  +   B  ][ A  +  B ][ A + B][ A   +   B  ]
      = [                        A       +       B                             ]


That's the general idea, but it raises some questions:

 - What size should each A block be?
 - What does it mean to "break A into blocks"?
 - How exactly do we "insert" each A block into B without it being an n^2 operation?
 - Merge each [A][B] combination? **Wasn't that what we were *already trying to do*?**

* * *

First let's answer the first question, because it's only fitting that we answer
them in order. Each A block should be of size √(A.length), which incidentally
means there will be √(A.length) *number* of A blocks as well. You'll see why
we use that size at the end of this chapter.

* * *

Next up, you don't *actually* break the array into anything. What you instead
do is keep track of the size of each block √(A.length), how many A blocks we
need (A.length/block_size), and the start and end of the A blocks within the
array. Each block is then defined *implicitly* as A.start + index * block_size,
where index is from 0 to (block_count - 1).

* * *

As for how to insert the A blocks into B, the obvious solution ([rotating]
(https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201.%20Tools.md)
the blocks to where they belong) is an n^2 operation. So that's no good.
What we'll have to do instead is break B into blocks too, then *block swap*
an A block with a B block to roll the A blocks through the array.

    1. break B into evenly-sized blocks too (the A blocks are numbered to show order)
    [ 0 ][ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ B ][ B ][ B ][ B ][ B ][ B ][ B ][] <- extra bit of B left over
      ^
      |__ first A block

    2. allow the A blocks to be moved out of order, then simply swap it with the next B block
    [ B ][ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 0 ][ B ][ B ][ B ][ B ][ B ][ B ][]
      ^                                  ^
      |__ first B block (swapped with [ 0 ])

    3. keep going until we find where we want to "drop" the smallest A block behind
       (A[first] <= B[last], where A is the minimum A block, or [ 0 ], and B is the block we just swapped over)
    [ B ][ B ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 0 ][ 1 ][ B ][ B ][ B ][ B ][ B ][]
                ^                        ^
                |__ the first A block needs to be block swapped here

    4. find the EXACT spot within B where the A block should go
       (using a binary search for A[first] <= B[index])
    [ B ][ B ][ 0 ][ 3 ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ B ][ B ][ B ][ B ][ B ][]
            ^
            |__ really, it SHOULD be here

    5. use a rotation to move the A block into its final position!
    [ B ][B][ 0 ][][ 3 ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ B ][ B ][ B ][ B ][ B ][]
          ^      ^
          |______|__ B was split into two parts

Note that after the rotation, A[first] is indeed <= the first B value that follows it.

As soon as we've found the exact spot where an A block should be, and have
rotated it into place in the array, we should immediately merge the previous
A block with any B values that follow it. So, to continue the above example:

    6. find the next A block to move into position
    [ B ][B][ 0 ][][ B ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ 3 ][ B ][ B ][ B ][ B ][]
                     ^                             ^
                     |__ [ 3 ] was here            |__ but it was swapped over to here

    (notice that we left [ 0 ] behind, while the rest of the A blocks keep rolling along)

    7. let's say for example that we want to drop the next smallest A block here
    [ B ][B][ 0 ][][ B ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ 3 ][ B ][ B ][ B ][ B ][]
                          ^                   ^
                                              |__ swap this with [ 4 ]

    8. once again, find the EXACT spot where [ 1 ] should be rotated into the previous B block
    [ B ][B][ 0 ][][ B ][ 1 ][ 5 ][ 6 ][ 2 ][ 4 ][ 3 ][ B ][ B ][ B ][ B ][]
                     ^
                     |__ in this example, it's going in the exact middle

    9. there!
    [ B ][B][ 0 ][B][ 1 ][B ][ 5 ][ 6 ][ 2 ][ 4 ][ 3 ][ B ][ B ][ B ][ B ][]
                  ^

At this point we would merge [ 0 ] with the B values between [ 0 ] and [ 1 ]
\(marked with a ^ above).

Once we merge the previous A block with the B values that follow it, that means
that part of the array is *completely finished being merged* (see the top of
this chapter) and we don't need to keep track of it anymore. In fact, we only
need to keep track of the starting point of the new A block we just dropped
behind, the starting point of the A blocks that are "rolling" along, and how
many A blocks are left. This does not change regardless of the number of A
blocks that need to be inserted – hence O(1) memory.

Anyway, this process repeats until there are no A blocks left, at which point
we merge the last A block with the remainder of the B array.

* * *

**But... what was accomplished?**

We went from needing to merge A and B to needing to merge an A block with
*some number* of B values. Isn't that the same thing?

Not necessarily. Let's allow ourselves to temporarily modify the array so that the
first A block actually contains the first unique values within A. So, for example:

    1. we want to merge these arrays:
    [ 1 1 1 2 2 3 3 4 4 5 5 5 5 5 5 6 ][ 2 2 3 3 3 4 4 5 5 6 7 8 8 9 9 9 10 ]

    2. the size of each A block should be √(A.length), which in this case is √16 = 4
    [ 1 1 1 2 ][ 2 3 3 4 ][ 4 5 5 5 ][ 5 5 5 6 ] [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
      ^     ^      ^   ^

    3. the first A block needs to contain the first unique values in A (marked with ^ above)
    [ 1 2 3 4 ][ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ] [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]

    4. now we only merge the remaining A blocks – the first one is reserved for other purposes
    [ 1 2 3 4 ]      [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]      [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]

The trick now is that since this area is large enough to hold the values of
any A block (seeing as how it's the same size), we can use it as the buffer
for the [Merging without overwriting the contents of the half-size buffer]
(https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md)
algorithm!

The merge process causes the items in the buffer to move out of order, but
since the values are unique we can just insertion sort them when we're finished
merging the A blocks. And then we can just redistribute them back to where they
belong in the array, which completes the sorting process.

* * *

**So close, but...**

That's essentially all there is to efficient in-place merging, but if you were
to try to implement the above directly you'd run into a problem: how are we
supposed to know which A block is the smallest, after we've already moved them
out of order from rolling them through the B blocks? That information isn't
stored anywhere at the moment, and since the A blocks might all have the same
values we can't just compare them to each other to find the smallest one. We
also can't allocate space to store this information since it'd ruin the point!

The next trick is to use *another* A block to store *another* set of unique
values. While the first block is used as a buffer for the merging, *these*
unique values will be used to "tag" each A block so we have some way of
comparing them to determine their order. Once we pull out the unique values,
loop over the remaining A blocks and swap the first value in each block
with one of the unique values from this second buffer.

    1. we already pulled out unique values for the first block, but now we need another one
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
                                ^              ^

    2. oops, we don't have enough unique values! Fortunately we can use B just as well:
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
                                                                                ^      ^     ^    ^

    3. notice that the last B block was resized, to make room for this reserved block
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 8 9 ][ 9 ]  [ 7 8 9 10 ]

    4. anyway, now tag the A blocks with these unique values, by swapping the first value with one from the buffer
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 8 9 ][ 9 ]  [ 7 8 9 10 ]
                   ^          ^          ^                                                    ^ ^ ^

    5. all done!
    [ 1 2 3 4 ]  [ 7 1 2 3 ][ 8 5 5 5 ][ 9 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 8 9 ][ 9 ]  [ 1 4 5 10 ]
                   ^          ^          ^                                                    ^ ^ ^

Since the first value of each block is needed to decide where to drop an A
block behind – A[first] <= B[last] – the value will need to be read from the
second internal buffer since it was swapped over to there. The first minimum
A block is at index 0 in the buffer, the next smallest is at index 1, etc.

Anyway, when we go to merge an A block with the B values that follow it, just
swap the first value in the A block back with its actual value in the buffer,
so the original data is restored. Unlike the first buffer, the values in this
one will still be in order by the time we're finished, so we never need to sort
this section. The values *will* need to be redistributed into the merged array
when we're finished, exactly the same as with the first block.

And of course once the smallest A block is dropped behind, you can find the
new smallest A block by looping over the first values in the remaining A
blocks and find the smallest one.

* * *

**Wait, another question: how do we "pull out" these unique values, without
it being an n^2 operation?**

With rotations, of course! The first step is to count out the unique
values we've found:

    1. let's say we need to find 5 unique values for our buffer
    [0 0 0 1 1 2 3 3 3 4 4 5 6 6 6 6 6 7 7 8 9 ... ]
     1     2   3 4     5

If we find enough values, we would then stop there and start rotating
at that index:

    2. we need to rotate this 4 to be next to the 3
    [0 0 0 1 1 2 3 [3 3 4] 4 5 6 6 6 6 6 7 7 8 9 ... ]
                        ^

    [0 0 0 1 1 2 3 [4 3 3] 4 5 6 6 6 6 6 7 7 8 9 ... ]
                    ^
                    |___ like so

    3. now rotate [3 4] to be next to the 2 (wait, it's already there!)
    [0 0 0 1 1 2 [3 4] 3 3 4 5 6 6 6 6 6 7 7 8 9 ... ]
                   ^

    4. now rotate [2 3 4] to be next to the 1
    [0 0 0 1 [1 2 3 4] 3 3 4 5 6 6 6 6 6 7 7 8 9 ... ]
                  ^

    5. lastly, rotate [1 2 3 4] to be next to the 0
    [0 0 [0 1 2 3 4] 1 3 3 4 5 6 6 6 6 6 7 7 8 9 ... ]
               ^

    6. here's our buffer!
    [0 1 2 3 4][0 0 1 3 3 4 5 6 6 6 6 6 7 7 8 9 ... ]
        ^

Redistributing the buffers back into [A+B] after merging is the same process,
but in reverse.

* * *

**What if we couldn't find enough unique values in A *or* B?**

If neither A nor B contain enough unique values to fill up the two required
blocks, then find as many unique values as possible and only use one internal
buffer of that size. We'll need to make the A and B blocks larger (meaning there
will be fewer blocks overall) so the buffer can still be used to tag the A
blocks. The new size for the blocks should be:

    block_size = A.length/(number of unique values we found) + 1

Since we won't have a second buffer available for merging the A and B blocks,
we'll need to use a different in-place merge algorithm. Fortunately, merging
arrays with similar values is the *easy* part! We can just binary search into
B and rotate A into place, like so:

    while (A.length > 0 and B.length > 0)
        find the first place in B where the first item in A needs to be inserted:
        mid = BinaryFirst(array, A.start, B)

        rotate A into place:
        amount = mid - (A.start + A.length)
        Rotate(array, amount, MakeRange(A.start, mid))

        calculate the new A and B ranges:
        B = MakeRange(mid, B.start + B.length)
        A = MakeRange(BinaryLast(array, A.start + amount, A), B.start)

So after dropping an A block behind and rotating it into the previous B block, instead of merging using the cache or the internal buffer, we would run the above algorithm instead. Everything else can remain exactly the same.

* * *

**And what if A can't be broken up into evenly sized blocks?**

All of these examples used perfect squares for the size of A (√16 = 4), but
what if A has... *17* items? Not a problem – just have the *first* A block
be unevenly sized, and don't roll it through the B blocks with the rest of
the A blocks. Once you drop the first evenly sized A block behind and rotate
it into the B block, that uneven A block should be merged with the B values
that follow it.

Since this unevenly sized A block won't be rolled along with the rest of the
A blocks, you do not need to "tag" it with a unique value from the buffer.

* * *

**And what about that last unevenly sized B block?**

If you get to that point and there are still A blocks left, you'll have to
*rotate* the remaining A blocks with that uneven-sized B block, rather than
*block swap*, so that the unevenly sized B block appears before those A blocks
in the array. You'll still need to check every remaining A block to see if it
needs to be rotated back into that B block, exactly the same as before.

* * *

**Aren't there still n^2 operations being used?**

Yes, yes there are. When we "drop" the smallest A block behind, we need to
use a linear search through the remaining A blocks to find the next smallest
one (O(n) operation performed n times = O(n^2)); and when we are finished
merging the A and B blocks and we're left with the two reserved blocks, we
have to apply an insertion sort to one of them (O(n^2) on its own).

* * *

**But!** keep in mind that since the size of each block is actually √(A.length),
performing an n^2 operation on a √n set of data ends up being O(n), or linear!
So the linear search checks √(A.length) items, √(A.length) number of times,
and the insertion sort is applied one time to a block of size √(A.length).
Both of those end up being O(A.length).

* * *

You should now have a fully-functional stable sorting algorithm that uses O(1)
memory! [However...]
(https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%204.%20Faster!.md)
