#include "compiler.h"

Compiler::Compiler(){
    hadError = false;
    panicMode = false;
    objects = nullptr;
};

Compiler::~Compiler(){
};

bool Compiler::compile(char* src){
    scanner = new Scanner(src);
    hadError = panicMode = false;

    advance();

    while (!match(TOKEN_EOF)){
        declaration();
    }

    emitByte(OP_RETURN);

    #ifdef DEBUG_PRINT_CODE
    if (!hadError){
        debugger.setChunk(currentChunk());
        debugger.debug("code");
        cout << "==========" << endl;
    }
    #endif

    delete scanner;
    return !hadError;
}

void Compiler::declaration(){
    if (match(TOKEN_VAR)){
        const_index_t global = parseVariable("Expect variable name.");
        if (match(TOKEN_EQUAL)){
            expression();
        } else {
            emitByte(OP_NIL);
        }
        consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
        emitBytes(OP_DEFINE_GLOBAL, global);
    } else {
        statement();
    }

    if (panicMode) synchronize();
}

void Compiler::statement(){
    switch (current.type) {
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
        default:
            expression();
            consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
            // A statement must not change the stack size.
            // This means that stack overflow can only happen during one statement.
            // We don't want a long program to be leaking stack values
            // everytime it wants to call a function for a side effect and discard the return value.
            emitByte(OP_POP);
    }
}

// returns an index into the constants array for the variable name as a string
const_index_t Compiler::parseVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(previous);
}

// returns an index into the constants array for the identifier name as a string
const_index_t Compiler::identifierConstant(Token name) {
    Value varName = createStringValue(name.start, name.length);
    return chunk->addConstant(varName);
}

void Compiler::expression() {
    parsePrecedence(PREC_ASSIGNMENT);
}

void Compiler::number(){
    double value = strtod(previous.start, nullptr);
    emitConstantAccess(NUMBER_VAL(value));
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

void Compiler::string(){
    // TODO: support escape sequences

    const char* chars = previous.start + 1;
    int length = previous.length - 2;
    emitConstantAccess(createStringValue(chars, length));
}

Value Compiler::createStringValue(const char* chars, int length){
    // TODO: since it allocates memory here, just exporting the code and constants array to a file will not be enough. you need to write all the data that the objects point to.
    return OBJ_VAL(copyString(strings, &objects, chars, length));
}

void Compiler::namedVariable(Token name, bool canAssign) {
    uint8_t global = identifierConstant(name);

    // If the variable is inside a high precedence expression, it has to be a get not a set.
    // Like a * b = c should be a syntax error not parse as a * (b = c).
    if (canAssign && match(TOKEN_EQUAL)){
        expression();
        emitBytes(OP_SET_GLOBAL, global);
    } else {
        emitBytes(OP_GET_GLOBAL, global);
    }

}


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

    bool canAssign = precedence <= PREC_ASSIGNMENT;

    switch (previous.type) {
        PREFIX_OP(TOKEN_NUMBER, number())
        PREFIX_OP(TOKEN_MINUS, unary(PREC_UNARY))
        PREFIX_OP(TOKEN_BANG, unary(PREC_NONE))
        PREFIX_OP(TOKEN_LEFT_PAREN, grouping())
        LITERAL(TOKEN_TRUE, OP_TRUE)
        LITERAL(TOKEN_FALSE, OP_FALSE)
        LITERAL(TOKEN_NIL, OP_NIL)
        case TOKEN_STRING:
            string();
            break;
        case TOKEN_IDENTIFIER:
            namedVariable(previous, canAssign);
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
            case TOKEN_EQUAL:
                errorAt(current, "Invalid assignment target");
                return;

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

// ==== Boring ====

void Compiler::emitByte(uint8_t byte){
    currentChunk()->write(byte, current.line);
}

void Compiler::emitBytes(uint8_t byte1, uint8_t byte2){
    emitByte(byte1);
    emitByte(byte2);
}

void Compiler::emitConstantAccess(Value value){
    // just in-case I call this by accident instead of writing the opcode myself.
    if (IS_BOOL(value)){
        if (AS_BOOL(value)){
            emitByte(OP_TRUE);
        } else {
            emitByte(OP_FALSE);
        }
        return;
    } else if (IS_NIL(value)){
        emitByte(OP_NIL);
        return;
    }

    // TODO: deduplicate the constants array so we'd write the old index instead of adding a new one with the same value.
    const_index_t location = chunk->addConstant(value);
    emitBytes(OP_GET_CONSTANT, location);
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
    return chunk;
}

void Compiler::setChunk(Chunk* chunkIn) {
    chunk = chunkIn;
}
