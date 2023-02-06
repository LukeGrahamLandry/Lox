#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "debug.h"
#include "value.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR
} InterpretResult;

#define STACK_MAX 256

class VM {
public:
    VM();
    ~VM();

    uint8_t* ip;

    InterpretResult interpret(Chunk* chunk);
    InterpretResult interpret(char *src);
    void setChunk(Chunk *chunk);
private:
    Debugger* debug;
    Chunk* chunk;
    Value stack[STACK_MAX];
    Value* stackTop;  // where the next value will be inserted

    InterpretResult run();
    void resetStack();
    void push(Value value);
    Value pop();
};

#endif