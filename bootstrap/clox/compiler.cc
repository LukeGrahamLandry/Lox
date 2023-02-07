#include "compiler.h"

Compiler::Compiler(Obj** objects){
    hadError = false;
    panicMode = false;
    this->objects = objects;

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
    emitConstant(NUMBER_VAL(value));
}

void Compiler::unary(Precedence precedence){
    TokenType operatorType = previous.type;
    parsePrecedence(precedence);  // vm: push value to stack

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);  // vm: pop it off, negate, put back
            break;
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        default:
            return;
    }
}

// (-1 + 2) * 3 - -4

void Compiler::parsePrecedence(Precedence precedence){
    advance();

    #define PREFIX_OP(token, methodCall) \
            case token:              \
                methodCall;          \
                break;

    #define LITERAL(token, opcode)   \
            case token:              \
                emitByte(opcode);    \
                break;

    switch (previous.type) {
        PREFIX_OP(TOKEN_NUMBER, number())
        PREFIX_OP(TOKEN_MINUS, unary(PREC_UNARY))
        PREFIX_OP(TOKEN_BANG, unary(PREC_NONE))
        PREFIX_OP(TOKEN_LEFT_PAREN, grouping())
        LITERAL(TOKEN_TRUE, OP_TRUE)
        LITERAL(TOKEN_FALSE, OP_FALSE)
        LITERAL(TOKEN_NIL, OP_NIL)
        case TOKEN_STRING:
            // TODO: support escape sequences
            // TODO: since it allocates memory here, just exporting the code and constants array to a file will not be enough. you need to write all the data that the objects point to.
            emitConstant(OBJ_VAL(copyString(objects, previous.start + 1, previous.length - 2)));
            break;
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

    #define DOUBLE_BINARY_INFIX_OP(operatorToken, operatorPrecedence, opcode, opcode2) \
            case operatorToken: {                                      \
                if (precedence > operatorPrecedence) return;           \
                advance();                                             \
                parsePrecedence((Precedence)(operatorPrecedence + 1)); \
                emitByte(opcode);                                      \
                emitByte(opcode2);                                     \
                break;                                                 \
            }

    for (;;){
        switch (current.type) {
            BINARY_INFIX_OP(TOKEN_MINUS, PREC_TERM, OP_SUBTRACT)
            BINARY_INFIX_OP(TOKEN_PLUS, PREC_TERM, OP_ADD)
            BINARY_INFIX_OP(TOKEN_SLASH, PREC_FACTOR, OP_DIVIDE)
            BINARY_INFIX_OP(TOKEN_STAR, PREC_FACTOR, OP_MULTIPLY)
            BINARY_INFIX_OP(TOKEN_EXPONENT, PREC_EXPONENT, OP_EXPONENT)
            BINARY_INFIX_OP(TOKEN_EQUAL_EQUAL, PREC_EQUALITY, OP_EQUAL)
            BINARY_INFIX_OP(TOKEN_LESS, PREC_COMPARISON, OP_LESS)
            BINARY_INFIX_OP(TOKEN_GREATER, PREC_COMPARISON, OP_GREATER)
            DOUBLE_BINARY_INFIX_OP(TOKEN_BANG_EQUAL, PREC_EQUALITY, OP_EQUAL, OP_NOT)
            DOUBLE_BINARY_INFIX_OP(TOKEN_GREATER_EQUAL, PREC_COMPARISON, OP_LESS, OP_NOT)
            DOUBLE_BINARY_INFIX_OP(TOKEN_LESS_EQUAL, PREC_COMPARISON, OP_GREATER, OP_NOT)

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
    #undef LITERAL
    #undef DOUBLE_BINARY_INFIX_OP
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
