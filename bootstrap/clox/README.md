

## Plan

Write the bytecode generation code in lox. 

I still want to be able to compile code written in the book's version of lox.
So any extra syntax (types) I add to give hints to the optimiser, should be a super set, and it should just guess if it's missing. 
Guessing is good anyway ex. var x = call(); if I know call always returns an int, why am I writing the type of the variable again.  
The 

Add static typing as a pre-compilation scan so the vm can still run bytecode with dynamic types. 
Then the programmer can know there will be no runtime cast exceptions.
I want the vm to still work with dynamic bytecode because seems like cheating to make an easier vm by limiting the programmer.
But doing it this way means that the vm can't optimise. It still needs to keep the type tag on every value 
and do the type checks at runtime for every operation even if we know it was statically checked, so it can never fail. 

Ways to optimise that:
1. 
have an opcode to promise the code was prechecked. (or better OP_META_FLAGS that says read the next byte so i can have 8 flags if i want).
so when it sees that byte, it stops keeping type tags for values and doing runtime checks. 
the compiler just puts that byte at the beginning if it does the type checks. 
annoying that I'd have to check the flag repeatedly but one bool check must be faster than 
the type checking and makes the values use so much less memory (including in bytecode form) that it must be worth it.
Have the checking mode field as a local variable to make sure its on the stack for faster.
2.
The vm actually is statically typed. Every variable it sees is one type forever.
When the code says to reassign a variable to a new type, what its actually doing is creating a new variable 
and changing which the name refers to. That's what gets written as byte code. 
This option moves more brains to the compiler and makes a smaller vm, so it seems better. 
It's not cheating because what the language can do doesn't change, just where it gets interpreted. 

Both of those or even forcing static types would save the 4 bytes for the 

## Optimising Variables

How the book does variables:
var name = 10; 
    - index = add to constants "name"
    - emit: OP_CONSTANT write(10)
    - emit: OP_DEFINE_GLOBAL index
Recall: 
    write(v): put Value(v), with wasted space for type tag, in the chunk's constant array then write byte(index) to the code array.
    They don't export the opcode and constant data as one stream of bytes. 
    VM OP_CONSTANT: read the next code byte as an index to get from the constants array and push to the stack.
Now we add VM OP_DEFINE_GLOBAL: 
    - name = next byte as index to constant array
    - in the globals hash table set: name = pop()
To retrieve, ex: name;
    - index = add to constants "name"
    - emit: OP_GET_GLOBAL index
VM OP_DEFINE_GLOBAL:
    - name = next byte as index to constant array
    - value = get name from globals hash table
    - push(value)

You shouldn't need to keep the actual string value. Each OP_GET_GLOBAL references an index into the constants array to get a hash code 
and that hash code is looked up in the table. You can't directly index into a globals array because what if the garbage collector makes 
stuff move around. But since local variables will be handled separately, maybe you never garbage collect the globals 
because you never know if something later will use them so then global variables can be direct indexes into an array. 
And local variables are indexes into that function's array which will get popped off the stack at the end. 
That's just reference counting tho.

A function is a variable whose value is a Chunk. 

Deduplication between function constant arrays and the main one? 
Should it deduplicate the constants arrays? You save memory but 
then have to spend time going through it each time a constant gets added 
but that's at compilation time so if its just interpreting source, don't 
but if you're compiling and saving the byte code, do extra work to make it shorter. 
Since it won't take more time at runtime. 

## Exporting a Chunk


