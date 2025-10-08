#Bong Compiler#

This compiler takes inspiration from the working of C. 

This is my second attempt at making a compiler for a imaginary language of mine. 


At first i wanted to create a language where you only do opertions using registers, i.e instead of working with higher level abstractions like strings and arrays, it would have been array of registers that we manipulate to perform all operations we require. 


I chickened out and am creating this instead. I will honestly agree that i havent 100% thought of all logic on my own, as i have used other compiler examples , just changed the logic to my liking. 

Currently, it works with a simple IR and flag for different ir levels. Only one ir level is implemented but more will be in the future.


It uses fasm as the assembly target and is linked by the compiler itself. 




*Future Planning*



Through recent digging on the internet, I have  identified several key improvements for the IR architecture. I plan to transition from address-based jumps to a label-based system, where labels mark jump targets and addresses are resolved during code generation rather than compilation. This approach simplifies implementation while enabling optimizations like constant folding and dead code elimination.
For constant folding, I propose an interpreter-based optimization that tracks which variables have compile-time-known values, folding operations when both operands are known, and resetting this knowledge at label boundaries to handle control flow correctly. For dead code elimination, I've prototyped a depth-first search algorithm that marks reachable instructions and removes unreachable code.
I'm particularly interested in learning more about optimization ordering, Static Single Assignment (SSA) form, register allocation strategies, and how professional compilers balance multiple optimization passes to achieve optimal results.

+ just making the error handling better, 


















**Fixing copmilaton errors ;-;**
