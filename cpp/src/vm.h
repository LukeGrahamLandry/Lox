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

class VM {
public:
    VM();
    ~VM();

    byte* ip;
    int exitCode;

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
    ObjString* produceString(const string& str);
    Value produceFunction(char *src);

    Memory gc;

//private:  // TODO
    Compiler compiler;

    Debugger debug;

    ostream* out;
    ostream* err;

    void resetStack();
    void push(Value value);
    Value pop();

    inline Value peek(int distance) {
        return gc.stackTop[-1 - distance];
    }

    inline Value peek() {
        return gc.stackTop[-1];
    }

    virtual void runtimeError(const string &message);
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
        return gc.frames[gc.frameCount - 1].closure->function->chunk;
    }

    bool callValue(Value value, int count);

    bool call(ObjClosure *closure, int argCount);
    void defineNative(const string& name, NativeFn function, int arity);

    ObjUpvalue* captureUpvalue(Value* local);
    void closeUpvalues(Value* last);
    bool invokeFromClass(ObjClass* klass, ObjString* name, int argCount);

    virtual void afterPrint();
};

#endif