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
    int assignments;
    bool isFinal;
    bool isCaptured;
} Local;

typedef struct {
    int continueTargetPosition;
    int startingScopeDepth;
    ArrayList<int> breakStatementPositions;
    ArrayList<int> continueStatementPositions;
} LoopContext;

typedef enum {
    TYPE_FUNCTION,
    TYPE_SCRIPT,
    TYPE_METHOD,
    TYPE_INITIALIZER
} FunctionType;

typedef struct {
    uint8_t index;
    bool isLocal;  // is index a stack offset or an upvalue index?
} Upvalue;

typedef struct {
    FunctionType type;
    ObjFunction* function;
    ArrayList<Local>* variableStack;
    int scopeDepth;
    ArrayList<Upvalue>* upvalues;  // TODO: dont heap allocate the lists. but c++ is hateful and i cant deal with destructors rn
} TargetFunction;

class Compiler {
public:
    Compiler(Memory& gc);
    ~Compiler();

    ObjFunction* compile(char *src);

    Memory& gc;
    ostream* err;  // TODO: unused because im using fprintf. newer c++ still doesnt seem to give me a format function for streams even though it says it does?
private:
    Token current;
    Token previous;
    bool hadError;
    bool panicMode;
    Scanner* scanner;
    bool currentHasSuper;

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
    int makeLocal(Token name);
    Token syntheticToken(const char* name);

    void unary();
    void method();
    void superAccess();
    void importNative(Token name);

    void string();
    Value createStringValue(const char* chars, int length);

    bool match(TokenType token);
    void declaration();
    void statement();
    void varStatement();
    void classDeclaration();
    void expressionStatement();
    void checkNotInBuffers(ArrayList<byte>* buffer);
    void emitEmptyReturn();

    bool check(TokenType type);

    void synchronize();

    byte identifierConstant(Token name);

    void namedVariable(Token name, bool canAssign);
    int resolveUpvalue(int funcIndex, Token* name);

    void beginScope();

    void endScope();

    void block();

    int declareLocalVariable();

    void defineLocalVariable();

    int parseLocalVariable(const char *errorMessage);

    bool identifiersEqual(Token name, Token name1);

    void emitPops(int count);

    int resolveLocal(TargetFunction& func, Token name);
    int addUpvalue(TargetFunction& func, uint8_t local, bool isLocal);

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
    ArrayList<Local>& getLocals(){
        return *functionStack.peekLast().variableStack;
    }
    int scopeDepth(){
        return functionStack.peekLast().scopeDepth;
    }

    int argumentList();

    void markRoots();
};


#endif
