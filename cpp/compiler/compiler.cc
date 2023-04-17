#include "compiler.h"

Compiler::Compiler(){
    hadError = false;
    panicMode = false;
    objects = nullptr;
};

Compiler::~Compiler(){
    if (bufferStack.count > 0){
        cerr << "Compiler had " << bufferStack.count << " unterminated bufferStack." << endl;
        for (int i=0; i < bufferStack.count; i++){
            delete bufferStack.get(i);
        }
    }
};

void Compiler::pushFunction(FunctionType currentFunctionType){
    TargetFunction func;
    func.variableStack = new ArrayList<Local>();
    func.type = currentFunctionType;
    func.function = NULL;
    func.function = newFunction();
    func.scopeDepth = 0;

    linkObjects(&objects, RAW_OBJ(func.function));

    Local local;
    local.depth = 0;
    local.name.start = "";
    local.name.length = 0;
    local.assignments = 0;
    local.isFinal = false;
    func.variableStack->push(local);

    functionStack.push(func);
}

ObjFunction *Compiler::popFunction() {
    TargetFunction func = functionStack.pop();
    delete func.variableStack;
    return func.function;
}

// A script is implicitly wrapped in a function.
// So the compiler can just produce a normal ObjFunction* and all the vm has to do to start is directly call it.
// This works elegantly since I have no special behaviour in the global scope.
// I might not even need to track that it's a special TYPE_SCRIPT not just a TYPE_FUNCTION.
// But it seems silly to throw away the information this early.
ObjFunction* Compiler::compile(char* src){
    scanner = new Scanner(src);
    hadError = panicMode = false;

    pushFunction(TYPE_SCRIPT);
    advance();

    while (!match(TOKEN_EOF)){
        declaration();
    }

    emitConstantAccess(NUMBER_VAL(0));
    emitByte(OP_RETURN);

    #ifdef COMPILER_DEBUG_PRINT_CODE
    if (!hadError){
        debugger.setChunk(currentChunk());
        debugger.debug("script");
        if (!Debugger::silent) cout << "==========" << endl;
    }
    #endif

    delete scanner;
    return hadError ? nullptr : popFunction();
}

void Compiler::declaration(){
    switch (current.type) {
        case TOKEN_FINAL:
        case TOKEN_VAR: {
            varStatement();
            break;
        }
        case TOKEN_FUN:
            funDeclaration();
            break;

        case TOKEN_IMPORT: {
            advance();
            while (check(TOKEN_IDENTIFIER)){
                parseLocalVariable("Expect identifier after 'import'.");
                defineLocalVariable();
                ObjString* name = copyString(strings, &objects, previous.start, previous.length);
                Value value;

                if (natives->get(name, &value)){
                    emitConstantAccess(value);
                } else {
                    errorAt(previous, "Invalid import");
                }

                if (!match(TOKEN_COMMA)) break;
            }
            consume(TOKEN_SEMICOLON, "Expect ';' after statement.");
            break;
        }
        default:
            statement();
            break;
    }

    if (panicMode) synchronize();
}

void Compiler::varStatement(){
    bool isFinal = false;
    if (match(TOKEN_FINAL)){
        isFinal = true;
        advance();
    }

    match(TOKEN_VAR);

    parseLocalVariable("Expect variable name.");

    (*locals()->peekLast()).isFinal = isFinal;
    if (match(TOKEN_EQUAL)){
        expression();
        (*locals()->peekLast()).assignments++;
    } else {
        emitByte(OP_NIL);
    }
    defineLocalVariable();
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
}

void Compiler::funDeclaration(){
    advance();
    parseLocalVariable("Expect function name.");
    defineLocalVariable();

    ObjString* name = copyString(strings, &objects, previous.start, previous.length);
    functionExpression(TYPE_FUNCTION, name);
}

void Compiler::beginScope(){
    functionStack.peekLast()->scopeDepth++;
}

void Compiler::endScope(){
    // Walk back through the stack and pop off everything in this scope
    int count = emitScopePops(scopeDepth() - 1);
    locals()->popMany(count);
    functionStack.peekLast()->scopeDepth--;
}

int Compiler::emitScopePops(int targetDepth){
    int localsCount = 0;
    for (int i = (int) locals()->count - 1; i >= 0; i--){
        Local check = locals()->get(i);
        if (check.depth <= targetDepth) {
            break;
        }

        localsCount++;
    }

    emitPops(localsCount);
    return localsCount;
}


void Compiler::block(){
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)){
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

void Compiler::emitPops(int count){
    // Future-proof if I increase stack size. I don't want to bother having a two byte operand version of POP.
    // Since I pop(1) so often, I have a specific opcode that doesn't waste a byte on the argument.
    while (count > 0){
        if (count == 1) {
            emitByte(OP_POP);
            count--;
        }
        else if (count > 255){
            emitBytes(OP_POP_MANY, 255);
            count -= 255;
        }
        else {
            emitBytes(OP_POP_MANY, count);
            count = 0;
        }
    }
}

void Compiler::statement(){
    switch (current.type) {
        case TOKEN_LEFT_BRACE:
            advance();
            beginScope();
            block();
            endScope();
            break;
        case TOKEN_IF:
            ifStatement();
            break;
        case TOKEN_WHILE:
            whileStatement();
            break;
        case TOKEN_FOR:
            forStatement();
            break;
        case TOKEN_PRINT:
            advance();
            expression();
            consume(TOKEN_SEMICOLON, "Expect ';' after value.");
            emitByte(OP_PRINT);
            break;
        case TOKEN_DEBUGGER:
            advance();
            consume(TOKEN_SEMICOLON, "Expect ';' after 'debugger'.");
            emitByte(OP_DEBUG_BREAK_POINT);
            break;
        case TOKEN_EXIT:
            advance();
            consume(TOKEN_SEMICOLON, "Expect ';' after 'exit'.");
            emitByte(OP_EXIT_VM);
            break;
        case TOKEN_RETURN: {
            advance();
            if (match(TOKEN_SEMICOLON)){
                emitByte(OP_NIL);
            } else {
                expression();
                consume(TOKEN_SEMICOLON, "Expect ';' after 'return'.");
            }
            emitByte(OP_RETURN);
            break;
        }
        case TOKEN_BREAK:
        case TOKEN_CONTINUE:
            breakOrContinueStatement(current.type);
            break;
        default:
            expressionStatement();
    }
}

void Compiler::emitGetAndCheckRedundantPop(OpCode emitInstruction, OpCode checkInstruction, int argument){
    // Optimisation: if the last instruction was a setter expression statement,
    // the stack just had the variable we want so why popping it off and putting it back.
    // This makes it faster and saves 3 bytes every timeMS: SET I POP GET I -> SET I.
    // This seems to break the assertion about statements not changing the stack size,
    // but really it's just rewriting "x = 1; print x;" as "print x = 1;"
    if (currentChunk()->getCodeSize() >= 3){
        bool justPopped = currentChunk()->getInstruction(-1) == OP_POP;
        bool wasSame = currentChunk()->getInstruction(-2) == argument;
        bool justSetLocal = currentChunk()->getInstruction(-3) == checkInstruction || currentChunk()->getInstruction(-3) == emitInstruction;
        if (justPopped && wasSame && justSetLocal){
            currentChunk()->popInstruction();  // remove the pop
            return;
        }
    }
    emitBytes(emitInstruction, argument);
}