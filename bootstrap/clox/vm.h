#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "compiler.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_HALT
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
    Compiler* compiler;
    Debugger* debug;
    Chunk* chunk;
    Value stack[STACK_MAX];
    Value* stackTop;  // where the next value will be inserted
    Obj* objects;

    InterpretResult run();
    void resetStack();
    void push(Value value);
    Value pop();

    Value peek(int distance);

    void runtimeError(const string &message);

    static bool isFalsy(Value value);

    static bool valuesEqual(Value right, Value left);

    void concatenate();
    void freeObjects();
    static void freeObject(Obj* object);
};

#endif