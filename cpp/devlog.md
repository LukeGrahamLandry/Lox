
## Feb 8

### Failing to access global variables across chunks

```
> var x = 0;
[...]
> x;
0000    | OP_GET_GLOBAL       0 'x'
Current Chunk Constants:
[x]
Allocated Heap Objects:
0x600003308040 [x]
0x60000330c040 [x]
Global Variables:
x = 0
Location in chunk: 1 / 4

Err: Undefined variable 'x'.[line 0] in script
```

The name strings are not getting correctly deduplicated. 
When the new chunk is compiled, it can't see the strings table from the last chunk. Because I was clearing its table every new repl line when it moved into the vm's table. 
Only `safeFindEntry` does the deduplication check, `safeAddAll` just `calls` put which accepts hash collisions and compares memory addresses.

Fixing `addAll` to use `safeSet` means `strings` only ends up with 1 version of the name 
but all the Obj Values in the new constants array still point to the memory address of 
its new obj instead of the deduplicated one. 

Not clearing the compiler's `strings` table works. Just means I have to make sure not to reuse compiler objects for chunks 
that should be exported separately. 

Since the adding things only flows one way, if the repl computes a string 
and then on the next line the compiler finds it as a constant, 
those two will have different memory address. Because the deduplication happens too late. 
The new "hihi" address won't be in the interpreter's `strings` set but 
the new constants will point to it. 

I'll just have them share the set for now. 

`compiler.strings` was just a `Set` but that meant that when I assigned it to `interpreter.strings`, 
it implicitly created a new one and called the copy constructor (that I didn't define so the data didn't get copied). 
Making `compiler.strings` a `Set*` and setting it to the address of `interpreter.strings` made interning actually work again.


### Just spamming empty lines into the repl eventually crashes

Somehow im ending up with a chunk whose code is just OP_POP. 
So that applies to an empty stack and decrements pointer back behind the beginning so how it's just fucking with random memory. 
And since there's no return it just keeps running through whatever bytes come next in memory and just doing shit. 
Then it eventually has a EXC_BAD_ACCESS when it tries to debug an OP_GET_CONSTANT but the next random byte is an out-of-bounds index. 

Adding a stack size check in pop() should fix that, but I still don't know how the compiler is generating invalid code. 

More reproducible version of  the problem;

```c
int main() {
    VM* test = new VM();
    for (int j=0;j<1000;j++){
        char line[1024];
        line[0] = '\n';
        line[1] = '\0';

        test->loadFromSource(line);
        test->run();
    }
    delete test;

    return 0;
```

The problem was here. bytecode1 != bytecode2. I have absolutely no idea what's going on. Making the compiler's debugger 
instance be on the stack instead of using `new` makes the problem go away. 

```c
void Debugger::setChunk(Chunk* chunkIn) {
    unsigned char* bytecode1 = chunkIn->getCodePtr();
    chunk = chunkIn;
    unsigned char* bytecode2 = chunkIn->getCodePtr();
}
```