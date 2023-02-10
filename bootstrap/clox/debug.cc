#include "debug.h"
#include "value.h"

Debugger::Debugger(Chunk* chunk){
    setChunk(chunk);
}

Debugger::Debugger() {
    chunk = nullptr;
    lastLine = -1;
}

void Debugger::setChunk(Chunk* chunkIn) {
    unsigned char* bytecode1 = chunkIn->getCodePtr();

    chunk = chunkIn;

    unsigned char* bytecode2 = chunkIn->getCodePtr();


}

void Debugger::debug(const string& name){
    cout << "== " << name << " ==" << endl;
    for (int offset=0; offset < chunk->getCodeSize();){
        offset = debugInstruction(offset);
    }
}

int Debugger::simpleInstruction(const string& name, int offset) {
    cout << name << endl;
    return offset + 1;
}

int Debugger::constantInstruction(const string& name, int offset) {
    uint8_t constantIndex = chunk->getCodePtr()[offset + 1];
    Value constantValue = chunk->getConstant(constantIndex);
    printf("%-16s %4d '", name.c_str(), constantIndex);
    printValue(constantValue);
    cout << "'" << endl;
    return offset + 2;
}

int Debugger::debugInstruction(int offset){
    int line = chunk->getLineNumber(offset);
    if (line == lastLine){
        printf("%04d    | ", offset);
    } else {
        printf("%04d %4d ", offset, line);
    }
    lastLine = line;

    #define SIMPLE(op) \
            case op:   \
                return simpleInstruction(#op, offset);

    #define CONSTANT(op) \
                case op:         \
                    return constantInstruction(#op, offset);

    #define NUMBER_ARG(op) \
            case op:       \
                printf("%-16s %4d \n", #op, chunk->getCodePtr()[offset + 1]); \
                return offset + 2;

    OpCode instruction = (OpCode) chunk->getCodePtr()[offset];
    switch (instruction){
        SIMPLE(OP_POP)
        SIMPLE(OP_RETURN)
        SIMPLE(OP_PRINT)
        SIMPLE(OP_ADD)
        SIMPLE(OP_SUBTRACT)
        SIMPLE(OP_MULTIPLY)
        SIMPLE(OP_DIVIDE)
        SIMPLE(OP_NEGATE)
        SIMPLE(OP_EXPONENT)
        SIMPLE(OP_TRUE)
        SIMPLE(OP_FALSE)
        SIMPLE(OP_NIL)
        SIMPLE(OP_NOT)
        SIMPLE(OP_EQUAL)
        SIMPLE(OP_GREATER)
        SIMPLE(OP_LESS)
        SIMPLE(OP_DEBUG_BREAK_POINT)
        SIMPLE(OP_EXIT_VM)
        SIMPLE(OP_ACCESS_INDEX)
        SIMPLE(OP_SLICE_INDEX)
        CONSTANT(OP_DEFINE_GLOBAL)
        CONSTANT(OP_GET_GLOBAL)
        CONSTANT(OP_SET_GLOBAL)
        CONSTANT(OP_GET_CONSTANT)
        NUMBER_ARG(OP_POP_MANY)
        NUMBER_ARG(OP_GET_LENGTH)
        NUMBER_ARG(OP_GET_LOCAL)
        NUMBER_ARG(OP_SET_LOCAL)
        default:
            cout << "Unknown Opcode (index=" << offset << ", value=" << (int) instruction << ")" << endl;
            return offset + 1;
    }
    #undef SIMPLE
    #undef CONSTANT
}
