#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <string>
#include "list.cc"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NEGATE,
    OP_RETURN
} OpCode;

class Chunk {
    public:
        Chunk();
        ~Chunk();
        void write(uint8_t byte, int line);
        void writeConstant(Value value, int line);

        ArrayList<uint8_t>* code;
        ArrayList<int>* lines;
        ValueArray* constants;
};

#endif
