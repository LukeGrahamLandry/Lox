#include "debug.h"
#include "value.h"

Debugger::Debugger(Chunk* chunk){
    setChunk(chunk);
}

Debugger::Debugger() {
    chunk = nullptr;
}

void Debugger::setChunk(Chunk *chunk) {
    this->chunk = chunk;
}

void Debugger::debug(const string& name){
    cout << "== " << name << " ==" << endl;
    for (int offset=0; offset < chunk->code->count;){
        offset = debugInstruction(offset);
    }
}

int Debugger::simpleInstruction(const string& name, int offset) {
    cout << name << endl;
    return offset + 1;
}

int Debugger::constantInstruction(const string& name, int offset) {
    uint8_t constantIndex = chunk->code->get(offset + 1);
    Value constantValue = chunk->constants->get(constantIndex);
    printf("%-16s %4d '", name.c_str(), constantIndex);
    printValue(constantValue);
    cout << "'" << endl;
    return offset + 2;
}

int Debugger::debugInstruction(int offset){
    int lastLine = 0;
    int line = chunk->lines->get(offset);
    if (offset > 0){
        lastLine = chunk->lines->get(offset - 1);
    }

    if (line == lastLine){
        printf("%04d    | ", offset);
    } else {
        printf("%04d %4d ", offset, line);
    }

    #define SIMPLE(op) \
            case op:   \
                return simpleInstruction(#op, offset);

    uint8_t instruction = chunk->code->get(offset);
    switch (instruction){
        SIMPLE(OP_RETURN)
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
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", offset);
        default:
            cout << "Unknown Opcode " << endl;
            return offset + 1;
    }
    #undef SIMPLE
}


