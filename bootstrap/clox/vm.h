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

        Debugger* debug;
        Chunk* chunk;
        uint8_t* ip;
        Value stack[STACK_MAX];
        Value* stackTop;  // where the next value will be inserted

        InterpretResult interpret(Chunk* chunk);
        InterpretResult run();
        void setChunk(Chunk *chunk);

    void resetStack();

    void push(Value value);

    Value pop();
};


#endif