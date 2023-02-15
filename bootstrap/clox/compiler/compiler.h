#ifndef clox_compiler_h
#define clox_compiler_h

#include "../chunk.h"
#include "../scanner.h"
#include "../common.h"
#include "../object.h"
#include "../table.h"

#ifdef COMPILER_DEBUG_PRINT_CODE
#include "../debug.h"
#endif

// Order matters! Enums are just numbers.
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_TERNARY,     // a ? b : c, fun
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

typedef struct {
    int continueTargetPosition;
    int startingScopeDepth;
    ArrayList<int> breakStatementPositions;
    ArrayList<int> continueStatementPositions;
} LoopContext;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT
} FunctionType;

typedef struct {
    FunctionType type;
    ObjFunction* function;
    ArrayList<Local>* variableStack;
    int scopeDepth;
} TargetFunction;

class Compiler {
public:
    Compiler();
    ~Compiler();

    ObjFunction* compile(char *src);

    Obj* objects;
    Set* strings;
private:
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Scanner* scanner;

    #ifdef COMPILER_DEBUG_PRINT_CODE
    Debugger debugger;
    #endif

    ArrayList<ArrayList<byte>*> bufferStack;
    ArrayList<LoopContext*> loopStack;
    ArrayList<TargetFunction> functionStack;

    void funDeclaration();
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
    void ifStatement();
    int emitJump(OpCode op);
    void patchJump(int fromLocation);
    void whileStatement();
    void forStatement();
    void breakOrContinueStatement(TokenType type);

    void grouping();
    void sequenceSliceExpression();
    void setBreakTargetAndPopActiveLoop();
    void setContinueTarget();

    void parsePrecedence(Precedence precedence);

    void unary(Precedence precedence);

    void string();
    Value createStringValue(const char* chars, int length);

    bool match(TokenType token);
    void declaration();
    void statement();
    void varStatement();
    void expressionStatement();
    void checkNotInBuffers(ArrayList<byte>* buffer);

    bool check(TokenType type);

    void synchronize();

    byte identifierConstant(Token name);

    void namedVariable(Token name, bool canAssign);

    void beginScope();

    void endScope();

    void block();

    int declareLocalVariable();

    void defineLocalVariable();

    int parseLocalVariable(const char *errorMessage);

    bool identifiersEqual(Token name, Token name1);

    void emitPops(int count);

    int resolveLocal(Token name);

    int emitJumpIfTrue();

    int emitJumpIfFalse();
    int getJumpTarget();
    int emitJumpUnconditionally();

    void patchLoop(int offset);
    ArrayList<byte>* pushBuffer();
    void popBuffer();
    void flushBuffer(ArrayList<byte>*& theBuffer);

    int emitScopePops(int targetDepth);

    void pushActiveLoop();

    void writeShort(int offset, uint16_t v);

    void functionExpression(FunctionType funcType, ObjString* name);

    void pushFunction(FunctionType currentFunctionType);
    ObjFunction* popFunction();
    ArrayList<Local>* locals(){
        return functionStack.peekLast()->variableStack;
    }
    int scopeDepth(){
        return functionStack.peekLast()->scopeDepth;
    }

    int argumentList();
};


#endif
