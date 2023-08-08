#include "compiler.h"

// advances until it finds a statement boundary.
void Compiler::synchronize() {
    panicMode = false;

    while (current.type != TOKEN_EOF) {
        if (previous.type == TOKEN_SEMICOLON) return;
        switch (current.type) {
            case TOKEN_CLASS:
            case TOKEN_FUN:
            case TOKEN_VAR:
            case TOKEN_FOR:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_PRINT:
            case TOKEN_RETURN:
                return;

            default:
                ; // Do nothing.
        }

        advance();
    }
}

void Compiler::emitByte(byte b){
    if (bufferStack.count == 0){
        currentChunk()->write(b, current.line);
    } else {
        bufferStack.peekLast()->push(b);
    }
}

void Compiler::emitBytes(byte byte1, byte byte2){
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::advance(){
    previous = current;
    for (;;) {
        current = scanner->scanToken();
        if (current.type != TOKEN_ERROR) break;

        errorAt(current, current.start);
    }
}

bool Compiler::match(TokenType token){
    if (current.type == token) {
        advance();
        return true;
    }
    return false;
}

bool Compiler::check(TokenType type) {
    return current.type == type;
}

void Compiler::consume(TokenType token, const char* message){
    if (!match(token)) errorAt(current, message);
}

void Compiler::errorAt(Token& token, const char *message){
    if (panicMode) return;
    panicMode = true;
    fprintf(stderr, "[line %d] Error", token.line);

    if (token.type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token.type == TOKEN_ERROR) {
        // Nothing.
    } else {
        fprintf(stderr, " at '%.*s'", token.length, token.start);
    }

    fprintf(stderr, ": %s\n", message);
    hadError = true;
}

Chunk* Compiler::currentChunk(){
    return functionStack.peekLast().function->chunk;
}

void Compiler::writeShort(int offset, uint16_t v){
    byte a = (v >> 8) & 0xff;
    byte b = v & 0xff;
    if (offset != -1){
        currentChunk()->setCodeAt(offset, a);
        currentChunk()->setCodeAt(offset + 1, b);
    } else {
        emitBytes(a, b);
    }
}

ArrayList<byte>* Compiler::pushBuffer(){
    ArrayList<byte>* buffer = new ArrayList<byte>();
    bufferStack.push(buffer);
    return buffer;
}

void Compiler::popBuffer(){
    bufferStack.pop();
}

void Compiler::flushBuffer(ArrayList<byte>*& buffer){
    checkNotInBuffers(buffer);

    for (int i=0;i<buffer->count;i++){
        emitByte((*buffer)[i]);
    }

    delete buffer;
    buffer = nullptr;
}

void Compiler::checkNotInBuffers(ArrayList<byte>* buffer){
    if (bufferStack.isEmpty()) return;
    if (buffer == nullptr){
        cerr << "Must not call flushBuffer twice." << endl;
        return;
    }

    uint i= bufferStack.count - 1;
    do {
        if (bufferStack[i] == buffer){
            cerr << "Compiler must call popBuffer before flushBuffer" << endl;
            if (i == bufferStack.count - 1) {
                // If you find yourself thinking that flushBuffer should just call popBuffer automatically,
                // so you can push then immediately pop, that's just normal emitting.
                // The buffer exists, so you can compile and save to emit later after compiling & emitting other stuff.
                // Its only meaningful if you compile more after popBuffer before flushBuffer.

                // This avoids an infinite loop of the buffer writing it itself.
                bufferStack.remove(i);
            } else {
                // This will only happen if I make a mistake and a function pushes more than it pops.
                // So an inner function pushes and leaks something to the caller
                // who expected to be popping the one they pushed, but it wasn't on the top anymore.
                cerr << "Compiler bufferStack must be treated as a stack" << endl;
            }
        }
        i--;
    } while (i != 0);
}
