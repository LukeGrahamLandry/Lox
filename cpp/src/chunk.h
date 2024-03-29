#ifndef clox_chunk_h
#define clox_chunk_h

#include "common.h"
#include <string>
#include "list.cc"
#include "value.h"
#include <fstream>


typedef byte const_index_t;

typedef enum {
    OP_INVALID = 0,  // zero initialized memory shouldn't be valid instructions
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
    OP_ACCESS_INDEX,
    OP_SLICE_INDEX,
    OP_GET_LENGTH,
    OP_GET_LOCAL,
    OP_SET_LOCAL,
    OP_LOAD_INLINE_CONSTANT,
    OP_JUMP,
    OP_LOOP,
    OP_JUMP_IF_FALSE,
    OP_CALL,
    OP_CLOSURE,
    OP_GET_UPVALUE,
    OP_SET_UPVALUE,
    OP_CLOSE_UPVALUE,
    OP_CLASS,
    OP_SET_PROPERTY,
    OP_GET_PROPERTY,
    OP_METHOD,
    OP_INVOKE,
    OP_INHERIT,
    OP_GET_SUPER,
    OP_SUPER_INVOKE,
} OpCode;

class Chunk {
    public:
        Chunk();
        ~Chunk();
        Chunk(const Chunk& other);
        void release(Memory& gc);
        void write(byte b, int line, Memory& gc);
        int getLineNumber(int tokenIndex);
        const_index_t addConstant(Value value, Memory& gc);
        void rawAddConstant(Value value, Memory& gc);
        int getCodeSize();
        unsigned char* getCodePtr();
        void printConstantsArray();
        Value getConstant(int index);
        unsigned char getInstruction(int offset);
        int popInstruction();
        int getConstantsSize();
        void setCodeAt(int index, byte value);

        static string opcodeNames[256];

        // TODO: why are these lists on the heap
        ArrayList<byte>* code;
private:
    ArrayList<int>* lines;
    ArrayList<Value>* constants;

    void setDone(Memory& gc);
};

void appendAsBytes(ArrayList<byte>* data, uint32_t number);
void appendAsBytes(ArrayList<byte>* data, double number);

#endif
