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

typedef struct {
    ObjFunction* function;
    uint8_t* ip;
    Value* slots;
} CallFrame;

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * 256)


class VM {
public:
    VM();
    ~VM();

    byte* ip;

    InterpretResult interpret(char* src);
    bool loadFromSource(char *src);
    void printDebugInfo();

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

    Set strings;
    Obj* objects;
private:
    Compiler compiler;
    Value stack[STACK_MAX];  // working memory. my equivalent of registers
    Value* stackTop;  // where the next value will be inserted
    CallFrame frames[FRAMES_MAX];  // it annoys me to have a separate bonus stack instead of storing return addresses in the normal value stack
    int frameCount;

    // a linked list of all Values with heap allocated memory, so we can free them when we terminate.
    // interned strings. prevents allocating separate memory for duplicated identical strings.
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
    void printStackTrace(ostream* output);

    static bool isFalsy(Value value);

    void concatenate();
    void freeObjects();

    int stackHeight();

    void loadInlineConstant();

    bool accessSequenceIndex(Value array, int index, Value *result);

    bool accessSequenceSlice(Value array, int startIndex, int endIndex, Value *result);

    double getSequenceLength(Value array);

    inline Chunk* currentChunk(){
        return frames[frameCount - 1].function->chunk;
    }

    bool callValue(Value value, int count);

    bool call(ObjFunction *function, int argCount);
};



#endif