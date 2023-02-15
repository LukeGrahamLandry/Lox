#include "compiler.h"


void Compiler::breakOrContinueStatement(TokenType type){
    advance();
    consume(TOKEN_SEMICOLON, "Expect ';' after statement.");
    if (loopStack.isEmpty()){
        errorAt(previous, "Can't use loop jump outside loop.");
        return;
    }
    LoopContext** ctx = loopStack.peekLast();
    emitScopePops((*ctx)->startingScopeDepth);
    int location = emitJumpUnconditionally();
    switch (type) {
        case TOKEN_BREAK:
            (*ctx)->breakStatementPositions.push(location);
            break;
        case TOKEN_CONTINUE:
            (*ctx)->continueStatementPositions.push(location);
            break;
    }
}


void Compiler::ifStatement(){
    match(TOKEN_IF);
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    int jumpOverThen = emitJumpIfFalse();
    emitByte(OP_POP);
    statement();

    // It feels like without an else clause you wouldn't need this tiny jump, but you need to be able to hop over the extra pop
    // Jump if false could just pop itself, but then you'd need a different no-pop one for and/or
    int jumpOverElse = emitJumpUnconditionally();
    patchJump(jumpOverThen);
    emitByte(OP_POP);
    if (match(TOKEN_ELSE)) {
        statement();
    }
    patchJump(jumpOverElse);
}

void Compiler::whileStatement() {
    int jumpToCondition = getJumpTarget();
    pushActiveLoop();
    setContinueTarget();
    match(TOKEN_WHILE);
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int jumpOverBody = emitJumpIfFalse();
    emitByte(OP_POP);
    statement();
    patchLoop(jumpToCondition);

    patchJump(jumpOverBody);
    emitByte(OP_POP);
    setBreakTargetAndPopActiveLoop();
}

void Compiler::forStatement() {
    beginScope();
    match(TOKEN_FOR);
    consume(TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");

    // Initializer.
    if (match(TOKEN_SEMICOLON)) {
        // None
    } else if (check(TOKEN_VAR) || check(TOKEN_FINAL)) {
        varStatement();
    } else {
        expressionStatement();
    }

    // Condition
    int jumpToCondition = getJumpTarget();
    if (!match(TOKEN_SEMICOLON)){
        expression();
        consume(TOKEN_SEMICOLON, "Expect ';' after loop condition.");
    } else {
        emitByte(OP_TRUE);
    }

    int jumpOverBody = emitJumpIfFalse();
    emitByte(OP_POP);

    // Increment
    ArrayList<byte>* incrementExpression = pushBuffer();
    if (!match(TOKEN_SEMICOLON)){
        expression();
        emitByte(OP_POP);
    }

    popBuffer();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");

    // Body
    pushActiveLoop();
    statement();
    setContinueTarget();
    flushBuffer(incrementExpression);
    patchLoop(jumpToCondition);

    // Done
    patchJump(jumpOverBody);
    emitByte(OP_POP);
    endScope();
    setBreakTargetAndPopActiveLoop();
}


void Compiler::pushActiveLoop(){
    LoopContext* ctx = new LoopContext;
    ctx->startingScopeDepth = scopeDepth();
    ctx->continueTargetPosition = 0;
    loopStack.push(ctx);
}

// Call at the location 'continue' should return to.
// Must be called after getJumpTarget.
void Compiler::setContinueTarget(){
    (*loopStack.peekLast())->continueTargetPosition = currentChunk()->getCodeSize();
}

// Call at the location 'break' should skip to.
// Must be after the pop on the condition.
void Compiler::setBreakTargetAndPopActiveLoop(){
    LoopContext* ctx = loopStack.pop();
    for (int i=0;i<ctx->breakStatementPositions.count;i++){
        int location = ctx->breakStatementPositions.get(i);
        patchJump(location);
    }
    for (int i=0;i<ctx->continueStatementPositions.count;i++){
        int fromLocation = ctx->continueStatementPositions.get(i);
        int jumpDistance = ctx->continueTargetPosition - fromLocation - 2;
        OpCode jumpType = OP_JUMP;
        if (jumpDistance < 0){
            jumpDistance *= -1;
            jumpType = OP_LOOP;
        }
        if (jumpDistance > UINT16_MAX) {
            errorAt(current, "Too much code to jump over.");  // TODO: long jump
        }
        currentChunk()->setCodeAt(fromLocation - 1, jumpType);
        writeShort(fromLocation, jumpDistance);
    }
    delete ctx;
}

// Called to jump FROM the current location
int Compiler::emitJump(OpCode instruction) {
    emitByte(instruction);
    emitBytes(0xff, 0xff);
    return getJumpTarget() - 2;
}

// Called to jump TO the current location
// <fromLocation> is the location that we're jumping FROM, where we'll patch in the current location in the bytecode
void Compiler::patchJump(int fromLocation) {
    int jumpDistance = getJumpTarget() - fromLocation - 2;
    if (jumpDistance > UINT16_MAX) {
        errorAt(current, "Too much code to jump over.");  // TODO: long jump
    }
    writeShort(fromLocation, jumpDistance);
}

int Compiler::emitJumpIfTrue() {
    int jumpOverSkip = emitJump(OP_JUMP_IF_FALSE);
    int skip = emitJump(OP_JUMP);
    patchJump(jumpOverSkip);
    return skip;
}

int Compiler::emitJumpIfFalse() {
    return emitJump(OP_JUMP_IF_FALSE);
}

int Compiler::emitJumpUnconditionally() {
    return emitJump(OP_JUMP);
}

// TODO: detect if jumping over buffer boundary and throw error. jumping within is fine cause its a delta
int Compiler::getJumpTarget(){
    return currentChunk()->getCodeSize();
}

void Compiler::patchLoop(int loopStart) {
    emitByte(OP_LOOP);
    int jumpDistance = getJumpTarget() - loopStart + 2;
    if (jumpDistance > UINT16_MAX) {
        errorAt(current, "Too much code to jump over.");  // TODO: long jump
    }

    writeShort(-1, jumpDistance);
}
