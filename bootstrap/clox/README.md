

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

## Plan

What I care about is short and fast bytecode given to the runtime. 
Move smarts to the compiler so the vm can be as simple as possible. 
I don't care about compile time. Faster code is worth more passes. 

Judge accessing variable undeclared variable as though all functions were inlined. 
ie. The call must be after definition of all variables used but the body can reference variables 
that are declared after the function definition as long as they're in the same scope.

Write the bytecode generation code in lox. 

I still want to be able to compile code written in the book's version of lox.
So any extra syntax (types) I push to give hints to the optimiser, should be a super set, and it should just guess if it's missing. 
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

Both of those or even forcing static types would save the space for the tag which might 4 bytes cause padding. 

The tag only exists for type checking. Obj would still need its tag, so it knows how to free. 
Need separate opcodes if they act on different types, like ADD vs CONCAT. 
The VM would just always cast to the type required by the operator so the Value struct doesn't need a ValueType field. 
== between different types would compile to false and for same types, take the ValueType as an extra argument. 

## Optimising Variables

Could have type tags for each thing in constants array packed in a separate array of longs, so it could look 
them up if it really wanted to. There's only 4 options and im afraid of the padding. 

A function is a variable whose value is a Chunk. 

Deduplication between function constant arrays and the main one? 
Should it deduplicate the constants arrays? You save memory but 
then have to spend time going through it each time a constant gets added 
but that's at compilation time so if its just interpreting source, don't 
but if you're compiling and saving the byte code, do extra work to make it shorter. 
Since it won't take more time at runtime. 

Guess the starting size for the hash table when parsing code. 

Why aren't classes like structs where all fields are just in order in an array and names correspond to indexes. 

Don't want to keep var name strings so just have a counter/random integer used instead. I can check for collisions at compile time 
but that doesn't work for dynamically loading code. Maybe it can only throw away names if it's compiling a whole scope at once. 

Should make sure I do importing files before I start messing with this, so I can test. 
Maybe scripts should explicitly export what they want to give to importers and the import keyword just 
runs the other script and pulls anything exported into our scope. 
Make sure you can do `export` as part of the variable definition. Maybe just use `public` to be the same as java. 

Other people's native calls need to keep the name string around>
But I can have an opcode for the standard library native calls where the compiler just puts the name's hash code 
and then since the vm knows all those names ahead of time i can have a table where those codes go to function pointers.
So two opcodes: OP_VM_NATIVE_CALL OP_FFI_NATIVE_CALL

Can do types from the bytecode by running through it and making a stack where the values are types. 
When you hit a condition, try both paths. 

### Line number tracking

run-length encoding is much better than the original but still feels dumb since the lines are almost always sequential
should be able to imply that the next is just one more but have an escape code encase how the compiling works changes.
feels like im slowly reinventing bytecode again. I could just use that,
put a chunk of bytecode to generate this list as a constant and if I need to show an error,
execute it then look up in the array. Good for when I want to write to file at least even if its overkill for runtime. 
don't want to recompute it every instruction while debugging, and it can't change so cache it. 
It's a lazily loaded value encoded as a pure function that gets cached. 
Every reference is inlined as `(name == nil ? name = computeName() : name)`, but if multiple references in same function, only the first needs the check. 
`lazy` modifier for a var. have it be context dependent. `lazy var` -> TOKEN_LAZY_VAR, `var` -> TOKEN_VAR, `lazy` -> TOKEN_IDENTIFIER

### Names

No global variables. I don't like the giant hash table in the sky.
Imports (and implied ones like native functions) take a stack slot.
Figure out a way of storing names of variables somewhere if it's told to save debug symbols.
Do that by default because it lets you have good error messages. Special compiler flag to omit names.
When generating bytecode it should tell you how much space each category takes
and remind you about the flags to optimise that but warn that it makes error messages and debugging worse.
Have a way for it to export just the name mapping information, so you can retroactively apply it to a bug report. 

### Bytecode Export

Instead of LOAD_INLINE_CONSTANT that just does one, so you have to repeat it for each. 
Have LOAD_INLINE_CONSTANT_ARRAY with a short argument for the length. 
Can grow the array to the correct length so a bit faster since no resize. 
Then it loops through that many times and loads them in sequence. 
