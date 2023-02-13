#include "debug.h"
#include "value.h"


bool Debugger::silent = false;

Debugger::Debugger(Chunk* chunk){
    setChunk(chunk);
}

Debugger::Debugger() {
    chunk = nullptr;
    lastLine = -1;
}

void Debugger::setChunk(Chunk* chunkIn) {
    chunk = chunkIn;
    inlineConstantCount = 0;
}

void Debugger::debug(const string& name){
    if (silent) return;

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
    byte constantIndex = chunk->getCodePtr()[offset + 1];
    if (constantIndex < chunk->getConstantsSize()){
        Value constantValue = chunk->getConstant(constantIndex);
        printf("%-16s %4d '", name.c_str(), constantIndex);
        printValue(constantValue);
        cout << "'" << endl;
    } else if (constantIndex < (chunk->getConstantsSize() + inlineConstantCount)){
        printf("%-16s %4d Inline\n", name.c_str(), constantIndex);
    }
    else {
        printf("%-16s %4d Out of range\n", name.c_str(), constantIndex);
    }

    return offset + 2;
}

int Debugger::jumpInstruction(const char* name, int sign, Chunk* chunk, int offset) {
    uint16_t jump = (uint16_t)(chunk->getCodePtr()[offset + 1] << 8);
    jump |= chunk->getCodePtr()[offset + 2];
    printf("%-16s %4d -> %d\n", name, offset, offset + 3 + sign * jump);
    return offset + 3;
}

int Debugger::debugInstruction(int offset){
    if (silent) return 0;

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
        case OP_JUMP:
            return jumpInstruction("OP_JUMP", 1, chunk, offset);
        case OP_JUMP_IF_FALSE:
            return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
        case OP_LOOP:
            return jumpInstruction("OP_LOOP", -1, chunk, offset);
        case OP_LOAD_INLINE_CONSTANT: {
            offset++;  // op
            unsigned char type = chunk->getCodePtr()[offset];
            offset++;  // type
            printf("%-16s %4d ", "OP_LOAD_INLINE_CONSTANT", inlineConstantCount);
            inlineConstantCount++;

            switch (type) {
                case 0: {
                    double value;
                    assert(sizeof(value) == 8);
                    memcpy(&value, &chunk->getCodePtr()[offset], sizeof(value));
                    offset += sizeof(value);
                    printf("num %4f \n", value);
                    break;
                }
                case (1 + OBJ_STRING): {
                    int length;
                    assert(sizeof(length) == 4);
                    memcpy(&length, &chunk->getCodePtr()[offset], sizeof(length));
                    offset += sizeof(length);
                    cout << "str '";
                    for (int i=0;i<length;i++){
                        cout << chunk->getCodePtr()[offset+i];
                    }
                    cout << "'" << endl;
                    offset += length;
                    break;
                }
                default:
                    printf("%-16s %d Invalid Value Type \n", "OP_LOAD_INLINE_CONSTANT", type);
                    break;
            }
            return offset;
        }
        default:
            cout << "Unknown Opcode (index=" << offset << ", value=" << (int) instruction << ")" << endl;
            return offset + 1;
    }
    #undef SIMPLE
    #undef CONSTANT
}
