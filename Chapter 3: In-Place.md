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
    [                               A+B                                    ]
    

That's the general idea, but it raises some questions:

&nbsp;&nbsp;• What size should each A block be?<br/>
&nbsp;&nbsp;• How exactly do we "insert" each block into B without it being an n^2 operation?<br/>
&nbsp;&nbsp;• Merge each [A][B] combination? <b>Wasn't that what we were <i>already trying to do</i>?</b>


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
