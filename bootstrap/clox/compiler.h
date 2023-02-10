#ifndef clox_compiler_h
#define clox_compiler_h

#include "chunk.h"
#include "scanner.h"
#include "common.h"
#include "object.h"
#include "table.h"

#ifdef COMPILER_DEBUG_PRINT_CODE
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
    PREC_INDEX,       // []
    PREC_CALL,        // . ()
    PREC_PRIMARY
} Precedence;

typedef struct {
    Token name;
    int depth;
    bool isFinal;
    int assignments;
} Local;

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

    #ifdef COMPILER_DEBUG_PRINT_CODE
    Debugger debugger;
    #endif
    ArrayList<Local> locals;
    int scopeDepth;

    void expression();
    void errorAt(Token& token, const char* message);
    void consume(TokenType token, const char* message);
    void emitByte(byte byte1);
    void emitBytes(byte byte1, byte byte2);
    void emitConstantAccess(Value value);
    Chunk* currentChunk();
    void advance();
    void emitGetAndCheckRedundantPop(OpCode emitInstruction, OpCode checkInstruction, int argument);
    void number();

    void grouping();
    void sequenceSliceExpression();

    void parsePrecedence(Precedence precedence);

    void unary(Precedence precedence);

    void string();
    Value createStringValue(const char* chars, int length);

    bool match(TokenType token);
    void declaration();
    void statement();

    bool check(TokenType type);

    void synchronize();

    byte identifierConstant(Token name);

    byte parseGlobalVariable(const char *errorMessage);

    void namedVariable(Token name, bool canAssign);

    void beginScope();

    void endScope();

    void block();

    void declareLocalVariable();

    void defineLocalVariable();

    void parseLocalVariable(const char *errorMessage);

    bool identifiersEqual(Token name, Token name1);

    void emitPops(int count);

    int resolveLocal(Token name);
};


#endif
