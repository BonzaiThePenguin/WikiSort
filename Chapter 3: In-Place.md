Chapter 3: In-place merging
==============

To reiterate, much of the work presented here is based on a paper called <a href="http://www.researchgate.net/publication/225153768_Ratio_Based_Stable_In-Place_Merging">"Ratio based stable in-place merging", by Pok-Son Kim and Arne Kutzner</a>. However, it uses a much simpler design.<br/><br/>

<b>The basic idea</b>

The core of efficient in-place merging relies on the fact that merging two sorted arrays, A and B, is equivalent to breaking A into evenly-sized chunks, inserting them into B, then merging each A block with any values from B that follow it. So instead of approaching the problem like this:

    [              A                  ][                   B               ]

We instead think of it like this:

    1. break A into evenly-sized blocks
    [ A ][ A ][ A ][ A ][ A ][ A ][ A ][                   B               ]
    
    2. insert them into B where they belong (so A[first] <= B[last])
    [ A ][  B  ][ A ][B][ A ][ A ][   B   ][ A ][  B ][ A ][B][ A ][   B   ]
    
    3. merge each [A][B] combination using a ...buffer?
    [ A ][  B  ]  [ A ][B]  [ A ]  [ A ][   B   ]  [ A ][  B ]  [ A ][B]  [ A ][   B   ]
    
    4. all finished!
    [ A  +  B  ]  [ A + B]  [ A ]  [  A  +   B  ]  [ A  +  B ]  [ A + B]  [ A   +   B  ]
      = [ A  +  B  ][ A + B][ A ][  A  +   B  ][ A  +  B ][ A + B][ A   +   B  ]
      = [                               A+B                                    ]
    

That's the general idea, but it raises some questions:

&nbsp;&nbsp;• What size should each A block be?<br/>
&nbsp;&nbsp;• How exactly do we "insert" each A block into B without it being an n^2 operation?<br/>
&nbsp;&nbsp;• Merge each [A][B] combination? <b>Wasn't that what we were <i>already trying to do</i>?</b>


First let's answer the first question, because it's only fitting that we answer them in order. Each A block should be of size √(A.length). You'll see why in a second.<br/><br/>

As for how to insert the A blocks into B, the obvious solution (<a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%201:%20Useful%20Tools.md">rotating</a> the blocks to where they belong) is an n^2 operation. So that's no good. What we'll have to do instead is also break B into blocks too, then <i>block swap</i> an A block with a B block to roll the A blocks through the array.

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

We went from needing to merge A and B with needing to merge an A block with a B block. Isn't that the same thing?

Not necessarily. Let's allow ourselves to temporarily modify the array so that the first A block actually contains the first unique values within A. So, for example:
    
    1. we want to merge these arrays:
    [ 1 1 1 2 2 3 3 4 4 5 5 5 5 5 5 6 ][ 2 2 3 3 3 4 4 5 5 6 7 8 8 9 9 9 10 ]
    
    2. the size of each A block should be √(A.length), which in this case is √16 = 4
    [ 1 1 1 2 ][ 2 3 3 4 ][ 4 5 5 5 ][ 5 5 5 6 ] [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]
      ^     ^      ^   ^
    
    3. the first A block needs to contain the first unique values in A (marked with ^ above)
    [ 1 2 3 4 ][ 1 1 2 3 ][ 4 5 5 5 ][ 5 5 5 6 ] [ 2 2 3 3 ][ 3 4 4 5 ][ 5 6 7 8 ][ 8 9 9 9 ][ 10 ]

Then we merge the other A blocks, but not the first one. The trick now is that since this area is large enough to hold the values of any A block, we can use it as the buffer for the <a href="https://github.com/BonzaiThePenguin/WikiSort/blob/master/Chapter%202:%20Merging.md">Merging without overwriting the contents of the half-size buffer</a> algorithm! The merge process causes the items in the buffer to move out of order, but since the values are unique we can just sort them when we're finished merging the A blocks.


==========================
<b>Sorry, this is still very much a work in progress!</b>

- problem: shifting the A blocks over is an n^2 operation
- solution: swap the first A block with the next B block

- problem: that causes the A blocks to be out of order
- solution: before we break A and B into blocks, shift the first sqrt(A.length) unique values over to the very left into its own block, then swap those values with the last value of each A block. this will give us ordering information

- problem: finding the minimum A block after swapping them out of order is an n^2 operation
- solution: limit the size of each block to sqrt(A.length), so an (sqrt(n))^2 operation is just O(n)!

- problem merging A with the following B blocks also has a bad O whatever. also, weren't we trying to SOLVE the problem of having to merge A and B together?
- solution: create another buffer with unique values, and use that as swap space for merging. this will cause the values to be all mixed up, but since the values are unique we can just insertion sort them afterwards

- problem: insertion sort is O(n^2)
- solution: again, since the blocks are limited to size sqrt(n), an n^2 operation is just O(n)
