

## Language Additions

- `debugger;` 
- `**` (exponent)

## Parsing Source Code

- It's just a massive switch statement.

## Compiling Expressions

- It's just a massive switch statement. 

The output of the compiler is a Chunk. It has an array of bytecode and an Array of constants found in the source code. 


Since the VM runs on a stack, operators are preceded by their operands. 

Any expression must start with a valid prefix, so it starts by switching over those. 

It knows it found a literal if the token is STRING, NUMBER, BOOLEAN, or NIL.
It uses the group of characters referenced in the source code array to create a value of the correct c type.
It puts a Value in the constants array and writes OP_GET_CONSTANT then the index to the bytecode.



## Value

The Value type (entries in the constants array) is a tagged union struct. The first field is an enum saying what type it is 
then the data is in the next field. 

- NIL: no data
- BOOLEAN: the data is 1 bit, true or false
- NUMBER: the data is double
- OBJ: the data is a pointer to an instance of the Obj struct.

### Obj

An Obj represents a block of dynamically allocated memory on the heap. Each Obj has a type tag 
and a pointer to the next obj (it's a linked list node) so the vm can clear them all when it's done. 

They also have additional data depending on their type. 

- OBJ_STRING: an array of chars and its length. Since c works on strings as char arrays, it's only a dereference away. 

All strings are interned. The compiler and the vm share a hash set. Whenever a new StringObj Value is created, 
it's looked up in the set. If its already there, the old one is used. So every StringObj with identical 
characters points to the same memory address. This saves memory and means you can do equality checks really quickly 
but adds some overhead to creating a string (like by concatenation) because you might hit the hash table's resize. 

## Executing Bytecode

- It's just a massive switch statement. 

## Plan

What I care about is short and fast bytecode given to the runtime. 
Move smarts to the compiler so the vm can be as simple as possible. 
I don't care about compile time. Faster code is worth more passes. 

Judge accessing variable undeclared variable as though all functions were inlined. 
ie. The call must be after definition of all variables used but the body can reference variables 
that are declared after the function definition as long as they're in the same scope.

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

Both of those or even forcing static types would save the space for the tag which might 4 bytes cause padding. 

The tag only exists for type checking. Obj would still need its tag, so it knows how to free. 
Need separate opcodes if they act on different types, like ADD vs CONCAT. 
The VM would just always cast to the type required by the operator so the Value struct doesn't need a ValueType field. 
== between different types would compile to false and for same types, take the ValueType as an extra argument. 

## Optimising Variables

How the book does variables:
var name = 10; 
    - index = add to constants "name"
    - emit: OP_GET_CONSTANT write(10)
    - emit: OP_DEFINE_GLOBAL index
Recall: 
    write(v): put Value(v), with wasted space for type tag, in the chunk's constant array then write byte(index) to the code array.
    They don't export the opcode and constant data as one stream of bytes. 
    VM OP_GET_CONSTANT: read the next code byte as an index to get from the constants array and push to the stack.
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

Could have type tags for each thing in constants array packed in a separate array of longs so it could look 
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


## Exporting a Chunk

constant array size: How many Values.
bytecode array size: How many bytes.
array of Values to be loaded to constants array. 
array of bytes that have all the data to be put in heap for Objs in constants array. 
array of bytecode. 

This could all be encoded as bytecode with a couple extra instructions: 
- GROW_CONSTANTS_ARRAY: faster to have this manually requested with the correct length at the beginning than making it an ArrayList. 
- LOAD_INLINE_CONSTANT: read the next few bytes as a Value into the constants array. 
  - If it's an obj, read the size of heap space to allocate then copy that many bytes into that space. 

That could even save the state of a program during execution, and it could start where it left off. 
Just have a JUMP after data loading that skips to where the ip was before. 

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

