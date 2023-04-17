## Language Additions

- `debugger;` 
- `exit;`
- `a ** b` (a to the power of b)
- Index and slice strings: `s[index]` or `s[start:end]`
- `break` and `continue`
- `condition ? a : b`

## Parsing Source Code

- It's just a massive switch statement.

## Compiling Expressions

- It's just a massive switch statement. 

The output of the compiler is a Chunk. It has an array of bytecode and an Array of constants found in the source code. 


Since the VM runs on a stack, operators are preceded by their operands. 

Any expression must start with a valid prefix, so it starts by switching over those. 

It knows it found a literal if the token is STRING, NUMBER, BOOLEAN, or NIL.
It uses the group of characters referenced in the source code array to create a value of the correct c type.
It puts a Value in the constants array and writes OP_GET_CONSTANT then its index to the bytecode.
Constants are deduplicated. When the compiler adds a new constant, it loops through all existing ones to check if that value is already there.

## Value

The Value type (entries in the constants array) is a tagged union struct. The first field is an enum saying what type it is 
then the data is in the next field. 

- NIL: no data
- BOOLEAN: the data is 1 bit, true or false
- NUMBER: the data is double
- OBJ: the data is a pointer to an instance of the Obj struct.

An Obj represents a block of dynamically allocated memory on the heap. Each Obj has a type tag 
and a pointer to the next obj (it's a linked list node) so the vm can clear them all when it's done. 

They also have additional data depending on their type. 

- OBJ_STRING: an array of chars and its length. Since c works on strings as char arrays, it's only a pointer dereference away. 
- OBJ_FUNCTION: a Chunk and its arity. 

All strings are interned. The compiler and the vm share a hash set. Whenever a new StringObj Value is created, 
it's looked up in the set. If its already there, the old one is used. So every StringObj with identical 
characters points to the same memory address. This saves memory and means you can do equality checks really quickly 
but adds some overhead to creating a string (like by concatenation) because you might hit the hash table's resize. 

## Executing Bytecode

- It's just a massive switch statement.

### Control Flow

```
if (condition){
    THEN;
} else {
    ELSE; 
}
```

The condition is evaluated, then OP_JUMP_IF_FALSE decides whether to jump over THEN to ELSE. 
The end of THEN has an OP_JUMP that skips over ELSE. 
The beginning of both THEN and ELSE have an OP_POP that removes the condition. 
I could have the pop at the very end where the jump over ELSE lands but that would mean that nested if statements 
would collect stack values that would only get cleaned up as they started terminating. 
It feels nicer to not leave mess around if we know it will never be used. 
But needing to jump over the extra pop even when there's no ELSE feels a bit ugly.

### Functions

Each function is a heap allocated obj. 

When we hit an OP_CALL, all the arguments have already been evaluated and are on the stack. 
The argument to the op has the number of args, so we know how far back to look for the value being called.
We create a new call frame referencing that function and save the beginning of its view into the stack. 
There's only one value stack, each call frame just points to a different slot to treat as the beginning. 
We save the current ip on the previous stack frame and update the vm's ip to point to the beginning of the function's chunk. 

Then execution continues as before with local variable indexes now relative to the new frame's stack pointer. 

When we hit an OP_RETURN, the value being returned is at the top of the stack, so we pop it off. 
The vm's stack pointer is set to the base of the current function which is the top of the previous one. 
The call frame is popped as well and the vm's ip is set back to the one saved in the previous frame. 
Then the return value is pushed back to the new stack top. 

The top level script is also just compiled into a function that runs in the same way. 
So the first stack slot is always the main script's ObjFunction and the vm terminates when it returns from the last call frame.

Since each ObjFunction has its own Chunk, the constant arrays aren't deduplicated. 
But maybe it's fine since Values are just pointers and big strings are interned separately anyway. 
It means chunks could be loaded into memory when they're called the first time which would be good for large programs. 
