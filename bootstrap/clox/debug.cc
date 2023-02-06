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

    uint8_t instruction = chunk->code->get(offset);
    switch (instruction){
        case OP_RETURN:
            return simpleInstruction("OP_RETURN", offset);
        case OP_ADD:
            return simpleInstruction("OP_ADD", offset);
        case OP_SUBTRACT:
            return simpleInstruction("OP_SUBTRACT", offset);
        case OP_MULTIPLY:
            return simpleInstruction("OP_MULTIPLY", offset);
        case OP_DIVIDE:
            return simpleInstruction("OP_DIVIDE", offset);
        case OP_NEGATE:
            return simpleInstruction("OP_NEGATE", offset);
        case OP_EXPONENT:
            return simpleInstruction("OP_EXPONENT", offset);
        case OP_CONSTANT:
            return constantInstruction("OP_CONSTANT", offset);
        default:
            cout << "Unknown Opcode " << endl;
            return offset + 1;
    }
}


