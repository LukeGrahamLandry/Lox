#include "compiler.h"

Compiler::Compiler(){
    hadError = false;
    panicMode = false;
    #ifdef DEBUG_PRINT_CODE
    debugger = new Debugger();
    #endif
};

Compiler::~Compiler(){
    #ifdef DEBUG_PRINT_CODE
    delete debugger;
    #endif
};

bool Compiler::compile(char* src){
    scanner = new Scanner(src);
    hadError = panicMode = false;

    advance();
    expression();
    consume(TOKEN_EOF, "Expect end of expression.");
    emitByte(OP_RETURN);

    #ifdef DEBUG_PRINT_CODE
    if (!hadError){
        cout << "Compiled: " << endl;
        cout << src << endl;
        debugger->setChunk(currentChunk());
        debugger->debug("code");
        cout << "==========" << endl;
    }
    #endif

    delete scanner;
    return !hadError;
}

void Compiler::expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

void Compiler::number(){
    double value = strtod(previous.start, nullptr);
    emitConstant(value);
}

void Compiler::unary(){
    TokenType operatorType = previous.type;
    parsePrecedence(PREC_UNARY);  // vm: push value to stack

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);  // vm: pop it off, negate, put back
            break;
        default:
            return;
    }
}

// (-1 + 2) * 3 - -4

void Compiler::parsePrecedence(Precedence precedence){
    advance();

    #define PREFIX_OP(token, method) \
            case token:              \
                method();            \
                break;

    switch (previous.type) {
        PREFIX_OP(TOKEN_NUMBER, number)
        PREFIX_OP(TOKEN_MINUS, unary)
        PREFIX_OP(TOKEN_LEFT_PAREN, grouping)
        default:
            errorAt(previous, "Expect expression.");
            break;
    }

    #define BINARY_INFIX_OP(operatorToken, operatorPrecedence, opcode) \
            case operatorToken: {                                      \
                if (precedence > operatorPrecedence) return;           \
                advance();                                             \
                parsePrecedence((Precedence)(operatorPrecedence + 1)); \
                emitByte(opcode);                                      \
                break;                                                 \
            }                                                          \

    for (;;){
        switch (current.type) {
            BINARY_INFIX_OP(TOKEN_MINUS, PREC_TERM, OP_SUBTRACT)
            BINARY_INFIX_OP(TOKEN_PLUS, PREC_TERM, OP_ADD)
            BINARY_INFIX_OP(TOKEN_SLASH, PREC_FACTOR, OP_DIVIDE)
            BINARY_INFIX_OP(TOKEN_STAR, PREC_FACTOR, OP_MULTIPLY)
            BINARY_INFIX_OP(TOKEN_EXPONENT, PREC_EXPONENT, OP_EXPONENT)
            // TODO: `condition ? then : else`
            //     ? is binary and : is unary
            //     bytes: THEN ELSE EXPR OP_CONDITIONAL
            //     case OP_CONDITIONAL:
            //          bool expr = pop();
            //          if (expr) {pop()} else {Value v = pop(); pop(); push(v)}
            default:
                return;
        }
    }

    #undef PREFIX_OP
    #undef BINARY_INFIX_OP
}


// Grouping exists to change precedence but doesn't actually have a unique runtime representation.
void Compiler::grouping(){
    expression();
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

// ==== Boring ====

void Compiler::emitByte(uint8_t byte){
    currentChunk()->write(byte, previous.line);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2){
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitConstant(Value value){
    currentChunk()->writeConstant(value, previous.line);
}

void Compiler::advance(){
    previous = current;
    for (;;) {
        current = scanner->scanToken();
        if (current.type != TOKEN_ERROR) break;

        errorAt(current, current.start);
    }
}

void Compiler::consume(TokenType token, const char* message){
    if (current.type == token) advance();
    else errorAt(current, message);
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
    return chunk;
}

void Compiler::setChunk(Chunk* chunk) {
    this->chunk = chunk;
}
