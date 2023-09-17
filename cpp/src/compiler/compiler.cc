#include "compiler.h"
#include <cassert>

Compiler::Compiler(Memory& gc) : gc(gc) {
    hadError = false;
    panicMode = false;
    err = &cerr;
    currentHasSuper = false;
};

Compiler::~Compiler(){
    loopStack.release(gc);
    functionStack.release(gc);
    if (bufferStack.count > 0){
        cerr << "Compiler had " << bufferStack.count << " unterminated bufferStack." << endl;
        for (int i=0; i < bufferStack.count; i++){
            bufferStack[i]->release(gc);
            delete bufferStack[i];
        }
    }
    bufferStack.release(gc);

};

void Compiler::pushFunction(FunctionType currentFunctionType){
    TargetFunction func;
    func.variableStack = new ArrayList<Local>();
    func.upvalues = new ArrayList<Upvalue>();
    func.type = currentFunctionType;
    func.function = NULL;
    func.function = gc.newFunction();
    func.scopeDepth = 0;

    functionStack.push(func, gc);

    // Reserve a stack slot. For methods, it holds the receiver (`this`). For functions, it holds the ObjClosure but is unused.
    const char* name = "";
    if (currentFunctionType == TYPE_METHOD || currentFunctionType == TYPE_INITIALIZER) {
        name = "this";
    }
    makeLocal(syntheticToken(name));
    defineLocalVariable();
}

// TODO: get rid of this cause i dont always call it
ObjFunction *Compiler::popFunction() {
    TargetFunction func = functionStack.pop();
    func.variableStack->release(gc);
    func.upvalues->release(gc);
    delete func.variableStack;
    delete func.upvalues;
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

    // Original Lox doesn't have explicit imports, so implicitly import clock
    importNative(syntheticToken("clock"));

    while (!match(TOKEN_EOF)){
        declaration();
    }

    emitConstantAccess(NUMBER_VAL(0));
    emitByte(OP_RETURN);

    #ifdef COMPILER_DEBUG_PRINT_CODE
    if (!hadError){
        debugger.setChunk(currentChunk());
        debugger.debug("script");
        if (!Debugger::silent) cerr << "==========" << endl;
    }
    #endif

    delete scanner;
    return hadError ? nullptr : popFunction();
}

void Compiler::declaration(){
    switch (current.type) {
        case TOKEN_FINAL:
        case TOKEN_VAR:
            varStatement();
            break;
        case TOKEN_FUN:
            funDeclaration();
            break;
        case TOKEN_CLASS:
            classDeclaration();
            break;
        case TOKEN_IMPORT: {
            advance();
            while (match(TOKEN_IDENTIFIER)){
                importNative(previous);

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

void Compiler::importNative(Token nameToken) {
    makeLocal(nameToken);
    defineLocalVariable();
    ObjString* nameStr = gc.copyString(nameToken.start, nameToken.length);
    Value value;

    if (gc.natives->get(nameStr, &value)){
        emitConstantAccess(value);
    } else {
        errorAt(nameToken, "Invalid import");
    }
}

void Compiler::varStatement(){
    bool isFinal = false;
    if (match(TOKEN_FINAL)){
        isFinal = true;
        advance();
    }

    match(TOKEN_VAR);

    parseLocalVariable("Expect variable name.");

    getLocals().peekLast().isFinal = isFinal;
    getLocals().peekLast().isCaptured = false;
    if (match(TOKEN_EQUAL)){
        expression();
        getLocals().peekLast().assignments++;
    } else {
        emitByte(OP_NIL);
    }
    defineLocalVariable();
    consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
}

// NAME(ARGS) { BODY }
void Compiler::funDeclaration(){
    advance();
    parseLocalVariable("Expect function name.");
    defineLocalVariable();

    ObjString* name = gc.copyString(previous.start, previous.length);
    functionExpression(TYPE_FUNCTION, name);
}

void Compiler::beginScope(){
    functionStack.peekLast().scopeDepth++;
}

void Compiler::endScope(){
    // Walk back through the stack and pop off everything in this scope
    int count = emitScopePops(scopeDepth() - 1);
    getLocals().popMany(count);
    functionStack.peekLast().scopeDepth--;
}

int Compiler::emitScopePops(int targetDepth){
    int localsCount = 0;
    for (int i = (int) getLocals().count - 1; i >= 0; i--){
        Local check = getLocals()[i];
        if (check.depth <= targetDepth) {
            break;
        }
        if (check.isCaptured) {
            emitByte(OP_CLOSE_UPVALUE);
        } else {
            emitByte(OP_POP);
        }
        localsCount++;
    }

    return localsCount;
}

void Compiler::classDeclaration(){
    bool prevHasSuper = currentHasSuper;  // Just use the callstack to save this.
    consume(TOKEN_CLASS, "unreachable");
    consume(TOKEN_IDENTIFIER, "Expect class name.");
    Token className = previous;
    int nameId = identifierConstant(className);
    declareLocalVariable();
    defineLocalVariable();
    emitBytes(OP_CLASS, nameId);

    beginScope();
    if (match(TOKEN_LESS)) {
        consume(TOKEN_IDENTIFIER, "Expect super class name.");
        Token superName = previous;
        if (identifiersEqual(className, superName)) {
            errorAt(superName, "A class can't inherit from itself.");
        }

        // Reserve a stack slot to hold the super class
        int superVarId = makeLocal(syntheticToken("super"));
        defineLocalVariable();

        namedVariable(superName, false);  // load super class on the stack. this stays as that variable ^
        namedVariable(className, false); // then the new class again. this gets popped off by OP_INHERIT
        emitByte(OP_INHERIT);
        currentHasSuper = true;
    } else {
        currentHasSuper = false;
    }

    namedVariable(className, false);  // Load the class value on the stack (above the super if any).

    consume(TOKEN_LEFT_BRACE, "Expect '{' before class body.");
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
        method();
    }
    consume(TOKEN_RIGHT_BRACE, "Expect '}' after class body.");

    // Don't need tbe class on the stack anymore.
    emitByte(OP_POP);
    currentHasSuper = prevHasSuper;
    endScope();
}

void Compiler::method() {
    consume(TOKEN_IDENTIFIER, "Expect function name.");

    int nameId = identifierConstant(previous);
    ObjString* name = gc.copyString(previous.start, previous.length);

    // This leaves the closure value on the top of the stack.
    if (name == gc.init){
        functionExpression(TYPE_INITIALIZER, name);
    } else {
        functionExpression(TYPE_METHOD, name);
    }

    emitBytes(OP_METHOD, nameId);
}

void Compiler::block(){
    while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)){
        declaration();
    }

    consume(TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

// TODO: bring this back. it needs to decide to close upvalues sometimes.
//void Compiler::emitPops(int count){
//    // Future-proof if I increase stack size. I don't want to bother having a two byte operand version of POP.
//    // Since I pop(1) so often, I have a specific opcode that doesn't waste a byte on the argument.
//    while (count > 0){
//        if (count == 1) {
//            emitByte(OP_POP);
//            count--;
//        }
//        else if (count > 255){
//            emitBytes(OP_POP_MANY, 255);
//            count -= 255;
//        }
//        else {
//            emitBytes(OP_POP_MANY, count);
//            count = 0;
//        }
//    }
//}

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
                emitEmptyReturn();
            } else {
                if (functionStack.peekLast().type == TYPE_INITIALIZER) {
                    errorAt(previous, "Can't return a value from an initializer.");
                }
                expression();
                consume(TOKEN_SEMICOLON, "Expect ';' after 'return'.");
                emitByte(OP_RETURN);
            }
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

void Compiler::emitEmptyReturn() {
    if (functionStack.peekLast().type == TYPE_INITIALIZER) {
        emitBytes(OP_GET_LOCAL, 0);  // Constructors return `this` implicitly.
    } else {
        emitByte(OP_NIL);  // Void functions return nil implicitly.
    }

    emitByte(OP_RETURN);
}

void Compiler::markRoots() {
    for (int i=0;i<functionStack.count;i++) {
        gc.markObject((Obj*) functionStack[i].function);
    }
}