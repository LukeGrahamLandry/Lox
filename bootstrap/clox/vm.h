#ifndef clox_vm_h
#define clox_vm_h

#include "chunk.h"
#include "debug.h"
#include "value.h"
#include "compiler/compiler.h"
#include "table.h"
#include <chrono>
#include "common.h"

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

    byte* ip;

    InterpretResult interpret(Chunk* chunk);
    bool loadFromSource(char *src);
    void setChunk(Chunk *chunk);
    void printDebugInfo();

    Chunk* getChunk(){
        return chunk;
    }

    InterpretResult run();


    void setOutput(ostream* target){
        out = target;
        err = target;
    }

    #ifdef VM_PROFILING
    static long instructionTimeTotal[256];
    static int instructionCount[256];
    #endif

    static void printTimeByInstruction();

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

    ostream* out;
    ostream* err;

    void resetStack();
    void push(Value value);
    Value pop();

    inline Value peek(int distance) {
        return stackTop[-1 - distance];
    }

    inline Value peek() {
        return stackTop[-1];
    }

    void runtimeError(const string &message);
    void runtimeError();

    static bool isFalsy(Value value);

    void concatenate();
    void freeObjects();

    int stackHeight();

    void loadInlineConstant();

    bool accessSequenceIndex(Value array, int index, Value *result);

    bool accessSequenceSlice(Value array, int startIndex, int endIndex, Value *result);

    double getSequenceLength(Value array);
};



#endif