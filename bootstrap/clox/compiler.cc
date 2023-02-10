#include "compiler.h"

Compiler::Compiler(){
    hadError = false;
    panicMode = false;
    objects = nullptr;
    scopeDepth = 0;
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

    #ifdef COMPILER_DEBUG_PRINT_CODE
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
    bool isFinal = false;
    switch (current.type) {
        case TOKEN_FINAL:
            isFinal = true;
            advance();
        case TOKEN_VAR: {
            match(TOKEN_VAR);

            parseLocalVariable("Expect variable name.");
            (*locals.peekLast()).isFinal = isFinal;
            if (match(TOKEN_EQUAL)){
                expression();
                (*locals.peekLast()).assignments++;
            } else {
                emitByte(OP_NIL);
            }
            defineLocalVariable();
            consume(TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
            break;
        }
        default:
            statement();
            break;
    }

    if (panicMode) synchronize();
}

void Compiler::beginScope(){
    scopeDepth++;
}

void Compiler::endScope(){
    // Walk back through the stack and pop off everything in this scope
    int localsCount = 0;
    for (int i=locals.count-1;i>=0;i--){
        Local check = locals.get(i);
        if (check.depth < scopeDepth) break;  // we reached the end of the scope

        localsCount++;
        locals.pop();
    }

    emitPops(localsCount);
    scopeDepth--;
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
const_index_t Compiler::parseGlobalVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    return identifierConstant(previous);
}

void Compiler::parseLocalVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);
    declareLocalVariable();

    // TODO: keep the name when in debug mode
    // identifierConstant(previous);
}

// for preventing self reference in a definition
void Compiler::declareLocalVariable(){
    if (locals.count >= 256) {  // TODO: another opcode for more but that means i have to grow the stack as well
        errorAt(previous, "Too many local variables in function.");
        return;
    }

    Local local;
    local.depth = -1;
    local.name = previous;
    local.assignments = 0;
    local.isFinal = false;

    for (int i=locals.count-1;i>=0;i--){
        Local check = locals.get(i);
        // since <locals> is a stack, the first time you see something of a higher scope, everything will be
        if (check.depth != -1 && check.depth < scopeDepth) break;
        if (identifiersEqual(check.name, local.name)) {
            errorAt(previous, "Already a variable with this name in this scope.");
        }
    }

    locals.push(local);
}

void Compiler::defineLocalVariable(){
    // Mark initialized
    (*locals.peekLast()).depth = scopeDepth;
    // NO-OP at runtime, it just flows over and the stack slot becomes a local
}

int Compiler::resolveLocal(Token name){
    for (int i=locals.count-1;i>=0;i--){
        Local check = locals.get(i);
        if (identifiersEqual(check.name, name)) {
            if (check.depth == -1) {
                errorAt(previous, "Can't read local variable in its own initializer.");
            }
            return i;
        }
    }
    return -1;
}

bool Compiler::identifiersEqual(Token a, Token b) {
    return a.length == b.length && memcmp(a.start, b.start, a.length) == 0;
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
    // TODO: need another opcode if i grow the stack and allow >256 locals
    int local = resolveLocal(name);

    if (local == -1){
        errorAt(previous, "Undeclared variable.");
        return;
    }

    // If the variable is inside a high precedence expression, it has to be a get not a set.
    // Like a * b = c should be a syntax error not parse as a * (b = c).
    if (canAssign && match(TOKEN_EQUAL)){
        Local* variable = locals.peek(local);
        if (variable->assignments++ != 0 && variable->isFinal) {
            errorAt(previous, "Cannot assign to final variable.");
            return;
        }
        expression();
        emitBytes(OP_SET_LOCAL, local);
    } else {
        emitGetAndCheckRedundantPop(OP_GET_LOCAL, OP_SET_LOCAL, local);
    }
}

void Compiler::emitGetAndCheckRedundantPop(OpCode emitInstruction, OpCode checkInstruction, int argument){
    // Optimisation: if the last instruction was a setter expression statement,
    // the stack just had the variable we want so why popping it off and putting it back.
    // This makes it faster and saves 3 bytes every time: SET I POP GET I -> SET I.
    // This seems to break the assertion about statements not changing the stack size,
    // but really it's just rewriting "x = 1; print x;" as "print x = 1;"
    if (chunk->getCodeSize() >= 3){
        bool justPopped = chunk->getInstruction(-1) == OP_POP;
        bool wasSame = chunk->getInstruction(-2) == argument;
        bool justSetLocal = chunk->getInstruction(-3) == checkInstruction || chunk->getInstruction(-3) == emitInstruction;
        if (justPopped && wasSame && justSetLocal){
            chunk->popInstruction();  // remove the pop
            return;
        }
    }
    emitBytes(emitInstruction, argument);
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
                errorAt(current, "Invalid assignment target.");
                return;
            case TOKEN_LEFT_SQUARE_BRACKET:
                if (precedence > PREC_INDEX) return;
                advance();  // consume [
                sequenceSliceExpression();
                break;

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

// TODO: should be easy to extend this with write opcodes instead of the read ones.
// Expects '[' already consumed.
void Compiler::sequenceSliceExpression(){
    if (check(TOKEN_COLON)){  // no starting index. default to beginning of sequence
        emitConstantAccess(NUMBER_VAL(0));
    } else {
        expression();  // the starting index
    }

    if (match(TOKEN_COLON)){  // a slice, not just one entry
        if (match(TOKEN_RIGHT_SQUARE_BRACKET)){  // no ending index. default to end of sequence
            emitBytes(OP_GET_LENGTH, 1);
        } else {
            expression();  // the last index of the range
            consume(TOKEN_RIGHT_SQUARE_BRACKET, "Expect ']' after sequence slice");
        }
        emitByte(OP_SLICE_INDEX);
    } else {  // just access the one index
        consume(TOKEN_RIGHT_SQUARE_BRACKET, "Expect ']' after sequence index");
        emitByte(OP_ACCESS_INDEX);
    }
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
    if (IS_BOOL(value)) AS_BOOL(value) ? emitByte(OP_TRUE) : emitByte(OP_FALSE);
    else if (IS_NIL(value)) emitByte(OP_NIL);
    else {
        const_index_t location = chunk->addConstant(value);
        emitBytes(OP_GET_CONSTANT, location);
    }
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
