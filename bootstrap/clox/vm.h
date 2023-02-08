#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "compiler.h"
#include "table.h"

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPILE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_EXIT,
    INTERPRET_DEBUG_BREAK_POINT
} InterpretResult;

#define STACK_MAX 256

class VM {
public:
    VM();
    ~VM();

    uint8_t* ip;

    InterpretResult interpret(Chunk* chunk);
    bool loadFromSource(char *src);
    void setChunk(Chunk *chunk);
    void printDebugInfo();

    Chunk* getChunk(){
        return chunk;
    }

    InterpretResult run();

private:
    Chunk* tempSavedChunk;
    Compiler compiler;
    Chunk* chunk;
    Value stack[STACK_MAX];  // working memory. my equivalent of registers
    Value* stackTop;  // where the next value will be inserted

    Obj* objects;  // a linked list of all Values with heap allocated memory, so we can free them when we terminate.
    Set strings;   // interned strings. prevents allocating separate memory for duplicated identical strings.
    Table globals;
    Debugger debug;

    void resetStack();
    void push(Value value);
    Value pop();

    Value peek(int distance);

    void runtimeError(const string &message);
    void runtimeError();

    static bool isFalsy(Value value);

    void concatenate();
    void freeObjects();
};

#endif