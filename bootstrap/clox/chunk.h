#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <string>
#include "list.h"
#include "value.h"

typedef enum {
    OP_CONSTANT,
    OP_RETURN
} OpCode;

class Chunk {
    public:
        Chunk();
        ~Chunk();
        void write(uint8_t byte, int line);
        int addConstant(Value value);

        ArrayList<uint8_t>* code;
        ArrayList<int>* lines;
        ValueArray* constants;
};

#endif
