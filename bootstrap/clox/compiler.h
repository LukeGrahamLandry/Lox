#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "scanner.h"
#include "common.h"
#include "object.h"
#include "table.h"

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
    Compiler();
    ~Compiler();

    bool compile(char *src);
    void setChunk(Chunk *pChunk);

    Obj* objects;
    Set* strings;
private:
    Chunk* chunk;
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Scanner* scanner;
    Debugger debugger;

    void expression();
    void errorAt(Token& token, const char* message);
    void consume(TokenType token, const char* message);
    void emitByte(uint8_t byte);
    void emitBytes(uint8_t byte1, uint8_t byte2);
    void emitConstantAccess(Value value);
    Chunk* currentChunk();
    void advance();

    void number();

    void grouping();

    void parsePrecedence(Precedence precedence);

    void unary(Precedence precedence);

    void string();
    Value createStringValue(const char* chars, int length);

    bool match(TokenType token);
    void declaration();
    void statement();

    bool check(TokenType type);

    void synchronize();

    uint8_t identifierConstant(Token name);

    uint8_t parseVariable(const char *errorMessage);

    void namedVariable(Token name, bool canAssign);
};


#endif
