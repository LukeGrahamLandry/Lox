#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "scanner.h"
#include "common.h"
#include "object.h"

#ifdef DEBUG_PRINT_CODE
#include "debug.h"
#endif

// Order matters! Enums are just numbers.
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * /
    PREC_EXPONENT,    // **
    PREC_UNARY,       // ! -
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

class Compiler {
public:
    Compiler(Obj** objects);
    ~Compiler();

    bool compile(char *src);
    void setChunk(Chunk *pChunk);
private:
    Chunk* chunk;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Scanner* scanner;
    Obj** objects;

    #ifdef DEBUG_PRINT_CODE
    Debugger* debugger;
    #endif


    void expression();
    void errorAt(Token& token, const char* message);
    void consume(TokenType token, const char* message);
    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    void emitConstant(Value value);
    Chunk* currentChunk();
    void advance();

    void number();

    void grouping();

    void parsePrecedence(Precedence precedence);

    void unary(Precedence precedence);

};


#endif
