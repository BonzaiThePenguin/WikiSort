Chapter 3: In-place
==============

To reiterate, much of the work presented here is based on a paper called <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">"Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner</a>. However, it uses a simpler design.<br/><br/>

<b>The basic idea</b>

The core of efficient in-place merging relies on the fact that merging two sorted arrays, A and B, is equivalent to breaking A into evenly sized blocks, inserting them into B, then merging each A block with any values from B that follow it. So instead of approaching the problem like this:

    [              A                  ][                   B               ]

We think of it like this:

    1. break A into evenly sized blocks
    [ A ][ A ][ A ][ A ][ A ][ A ][ A ][                   B               ]
    
    2. insert them into B where they belong (so A[first] <= B[last])
    [ A ][  B  ][ A ][B][ A ][ A ][   B   ][ A ][  B ][ A ][B][ A ][   B   ]
    
    3. merge each [A][B] combination
    [ A ][  B  ]  [ A ][B]  [ A ]  [ A ][   B   ]  [ A ][  B ]  [ A ][B]  [ A ][   B   ]
    
    4. all finished!
    [ A  +  B  ]  [ A + B]  [ A ]  [  A  +   B  ]  [ A  +  B ]  [ A + B]  [ A   +   B  ]
      = [ A  +  B  ][ A + B][ A ][  A  +   B  ][ A  +  B ][ A + B][ A   +   B  ]
      = [                               A+B                                    ]


That's the general idea, but it raises some questions:

&nbsp;&nbsp;• What size should each A block be?<br/>
&nbsp;&nbsp;• How exactly do we "insert" each A block into B without it being an n^2 operation?<br/>
&nbsp;&nbsp;• Merge each [A][B] combination? <b>Wasn't that what we were <i>already trying to do</i>?</b><br/><br/>

First let's answer the first question, because it's only fitting that we answer them in order. Each A block should be of size √(A.length), which incidentally means there will be √(A.length) <i>number</i> of A blocks as well. You'll see why we use that size in a second.<br/><br/>

As for how to insert the A blocks into B, the obvious solution (<a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Tools.md">rotating</a> the blocks to where they belong) is an n^2 operation. So that's no good. What we'll have to do instead is break B into blocks too, then <i>block swap</i> an A block with a B block to roll the A blocks through the array.

    1. break B into evenly-sized blocks too (the A blocks are numbered to show order)
    [ 0 ][ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ B ][ B ][ B ][ B ][ B ][ B ][ B ][] <- extra bit of B left over
      ^
      |__ first A block
      
    2. allow the A blocks to be moved out of order, then simply swap it with the next B block
    [ B ][ 1 ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 0 ][ B ][ B ][ B ][ B ][ B ][ B ][]
      ^
      |__ first B block (swapped with A)
    
    3. keep going until we find where we want to "drop" the smallest A block behind
    [ B ][ B ][ 2 ][ 3 ][ 4 ][ 5 ][ 6 ][ 0 ][ 1 ][ B ][ B ][ B ][ B ][ B ][]
                ^
                |__ the first A block needs to be block swapped here
    
    4. find the EXACT spot within B where the A block should go
    [ B ][ B ][ 0 ][ 3 ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ B ][ B ][ B ][ B ][ B ][]
            ^
            |__ really, it SHOULD be here
    
    5. use a rotation to move the A block into its final position!
    [ B ][][ 0 ][][ 3 ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ B ][ B ][ B ][ B ][ B ][]
          ^      ^
          |______|__ B was split into two parts

As soon as we find the exact spot where an A block should be, and rotate it into place in the array, we should immediately merge the previous A block with any B values that follow it. So, to continue the above example:

    6. find the next A block to move into position
    [ B ][][ 0 ][][ B ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ 3 ][ B ][ B ][ B ][ B ][]
                    ^                             ^
                    |__ [ 3 ] was here            |__ but it was swapped over to here
    
    7. drop the smallest A block behind again, which is [ 1 ] this time
    [ B ][][ 0 ][][ B ][ 4 ][ 5 ][ 6 ][ 2 ][ 1 ][ 3 ][ B ][ B ][ B ][ B ][]
                                             ^
                                             |__ swap this with [ 4 ]
    
    8. once again, find the EXACT spot where [ 1 ] should be rotated into the previous B block
    [ B ][][ 0 ][][ B ][ 1 ][ 5 ][ 6 ][ 2 ][ 4 ][ 3 ][ B ][ B ][ B ][ B ][]
                    ^
                    |__ in this example, it's going in the exact middle
    
    9. there!
    [ B ][][ 0 ][B][ 1 ][B ][ 5 ][ 6 ][ 2 ][ 4 ][ 3 ][ B ][ B ][ B ][ B ][]
                 ^

At this point we would merge [ 0 ] with the B values between [ 0 ] and [ 1 ] \(marked with a ^ above). This process repeats until there are no A blocks left, at which point we merge it with the remainder of the B array.

==========================
<b>But... what was accomplished?</b>

We went from needing to merge A and B to needing to merge an A block with <i>some number</i> of B values. Isn't that the same thing?

Not necessarily. Let's allow ourselves to temporarily modify the array so that the first A block actually contains the first unique values within A. So, for example:
    
    1. we want to merge these arrays:
    [ 1 1 1 2 2 3 3 4 4 5 5 5 5 5 5 6 ][ 2 2 3 3 3 4 4 5 5 6 7 8 8 9 9 9 10 ]
    
    2. the size of each A block should be √(A.length), which in this case is √16 = 4
    [ 1 1 1 2 ][ 2 3 3 4 ][ 4 5 5 5 ][ 5 5 5 6 ] [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
      ^     ^      ^   ^
    
    3. the first A block needs to contain the first unique values in A (marked with ^ above)
    [ 1 2 3 4 ][ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ] [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
    
    4. now we only merge the remaining A blocks – the first one is reserved for other purposes
    [ 1 2 3 4 ]      [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]      [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]

The trick now is that since this area is large enough to hold the values of any A block (seeing as how it's the same size), we can use it as the buffer for the <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md">Merging without overwriting the contents of the half-size buffer</a> algorithm!

The merge process causes the items in the buffer to move out of order, but since the values are unique we can just insertion sort them when we're finished merging the A blocks. And then we can just redistribute them back to where they belong in the array, which completes the sorting process.

==========================
<b>So close, but...</b>

That's essentially all there is to efficient in-place merging, but if you were to try to implement the above directly you'd run into a problem: how are we supposed to know which A block is the smallest, after we've already moved them out of order from rolling them through the B blocks? That information isn't stored anywhere at the moment, and since the A blocks might all have the same values we can't just compare them to each other to find the smallest one. We also can't allocate space to store this information since it'd ruin the point!

The next trick is to use <i>another</i> A block to store <i>another</i> set of unique values. While the first block is used as a buffer for the merging, <i>these</i> unique values will be used to "tag" each A block so we have some way of comparing them to determine their order. Once we pull out the unique values, loop over the remaining A blocks and swap the <i>second</i> value in each block with one of the unique values from this second buffer.

    1. we already pulled out unique values for the first block, but now we need another one
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ] [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
                              ^              ^
    
    2. oops, we don't have enough unique values! Fortunately we can use B just as well:
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
                                                                                ^      ^     ^    ^
    
    3. notice that the last B block was resized, to make room for this reserved block
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 8 9 ][ 9 ]  [ 7 8 9 10 ]
    
    4. anyway, now tag the A blocks with these unique values, by swapping the second value with one from the buffer
    [ 1 2 3 4 ]  [ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 8 9 ][ 9 ]  [ 7 8 9 10 ]
                     ^          ^          ^                                                  ^ ^ ^
    
    5. all done!
    [ 1 2 3 4 ]  [ 1 7 2 3 ][ 4 8 5 5 ][ 5 9 5 6 ]  [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 8 9 ][ 9 ]  [ 1 5 5 10 ]
                     ^          ^          ^                                                  ^ ^ ^

The reason we tag the second value of each A block, rather than the first or last, is because we use (A[first] <= B[last]) to decide where to leave the smallest A block. We currently don't use the last value for anything, but it could be useful for detecting contiguous A blocks (the last value of one A block equals the first value of the next, meaning B won't break them apart).

Anyway, when we go to merge an A block with the B values that follow it, just swap the second value in the A block back with its actual value in the buffer, so the original data is restored. Unlike the first buffer, the values in this one will stil be in order by the time we're finished, so we never need to sort this section. The values <i>will</i> need to be redistributed into the merged array when we're finished, exactly the same as with the first block.


============================
<b>What if we couldn't find enough unique values in A <i>or</i> B?</b>

If neither A nor B contain enough unique values to fill up the two required blocks, then obviously we can't do any of the above. Fortunately, merging arrays with similar values is the <i>easy</i> part! We can just binary search into B and rotate A into place, like so:

    while (A.length > 0 && B.length > 0)
        // find the first place in B where the first item in A needs to be inserted
        mid = BinaryFirst(array, A.start, B)
        
        // rotate A into place
        amount = mid - (A.start + A.length)
        Rotate(array, amount, RangeBetween(A.start, mid))
        
        // calculate the new A and B ranges
        B = RangeBetween(mid, B.start + B.length)
        A = RangeBetween(BinaryLast(array, A.start + amount, A), B.start)

============================
<b>Aren't there still n^2 operations being used?</b>

Yes, yes there are. When we "drop" the smallest A block behind, we need to use a linear search through the remaining A blocks to find the next smallest one (O(n) operation performed n times = O(n^2)); and when we are finished merging the A and B blocks and we're left with the two reserved blocks, we have to apply an insertion sort to one of them (O(n^2) on its own).<br/><br/>

<b>But!</b> keep in mind that since the size of each block is actually √(A.length), performing an n^2 operation on a √n set of data ends up being O(n), or linear! So the linear search checks √(A.length) items, √(A.length) number of times, and the insertion sort is applied one time to a block of size √(A.length). Both of those end up being O(A.length).


============================
<b>That's it!</b> The <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/WikiSort.c">provided implementation</a> has additional code for avoiding unnecessary operations and taking advantage of a local cache, but it is otherwise a direct implementation of the above concepts. Hopefully this information has proven valuable, and you can use your newfound knowledge to add your own optimizations and improvements. That way, everyone gets a better sort!
