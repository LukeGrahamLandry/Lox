#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <string>
#include "list.cc"
#include "value.h"
#include <fstream>


typedef byte const_index_t;

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
    OP_POP_MANY,
    OP_DEFINE_GLOBAL,
    OP_DEBUG_BREAK_POINT,
    OP_EXIT_VM,
    OP_GET_GLOBAL,
    OP_SET_GLOBAL,
    OP_ACCESS_INDEX,
    OP_SLICE_INDEX,
    OP_GET_LENGTH,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_LOAD_INLINE_CONSTANT
} OpCode;

class Chunk {
    public:
        Chunk();
        ~Chunk();
        Chunk(const Chunk& other);
        Chunk(ArrayList<byte>* exportedBinaryData);
        void write(byte byte, int line);
        int getLineNumber(int tokenIndex);
        const_index_t addConstant(Value value);
        void rawAddConstant(Value value);
        int getCodeSize();
        unsigned char* getCodePtr();
        void printConstantsArray();
        Value getConstant(int index);
        unsigned char getInstruction(int offset);
        int popInstruction();
        ArrayList<byte>* exportAsBinary();
        void exportAsBinary(const char* path);
        int getConstantsSize();

        static Chunk* importFromBinary(const char* path);
private:
    ArrayList<byte>* code;
    ArrayList<int>* lines;
    ValueArray* constants;

    uint32_t getExportSize();
};

void appendAsBytes(ArrayList<byte>* data, uint32_t number);
void appendAsBytes(ArrayList<byte>* data, double number);

#endif
