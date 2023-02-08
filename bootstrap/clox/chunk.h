#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <string>
#include "list.cc"
#include "value.h"


typedef uint8_t const_index_t;

typedef enum {
    OP_GET_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_NOT,
    OP_RETURN,
    OP_EXPONENT,
    OP_PRINT,
    OP_POP,
    OP_DEFINE_GLOBAL,
    OP_DEBUG_BREAK_POINT,
    OP_EXIT_VM,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL
} OpCode;

class Chunk {
    public:
        Chunk();
        ~Chunk();
        Chunk(const Chunk& other);
        void write(uint8_t byte, int line);
        int getLineNumber(int tokenIndex);
        const_index_t addConstant(Value value);
        int getCodeSize();
        unsigned char* getCodePtr();
        void printConstantsArray();
        Value getConstant(int index);
private:
    ArrayList<uint8_t>* code;
    ArrayList<int>* lines;
    ValueArray* constants;
};

#endif
