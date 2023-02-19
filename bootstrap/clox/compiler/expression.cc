#include "compiler.h"

void Compiler::expressionStatement(){
    expression();
    consume(TOKEN_SEMICOLON, "Expect ';' after expression.");
    // A statement must not change the stack size.
    // This means that stack overflow can only happen during one statement.
    // We don't want a long program to be leaking stack values
    // everytime it wants to call a function for a side effect and discard the return value.
    emitByte(OP_POP);
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
        case TOKEN_FUN: {
            if (check(TOKEN_IDENTIFIER)) break;
            std::string n = "lambda:" + to_string(previous.line);
            ObjString* name = copyString(strings, &objects, n.c_str(), (int) n.length());
            functionExpression(TYPE_FUNCTION, name);
            break;
        }
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
            case TOKEN_AND: {
                int jumpOverRight = emitJumpIfFalse();
                emitByte(OP_POP);
                parsePrecedence(PREC_AND);
                patchJump(jumpOverRight);
                break;
            }
            case TOKEN_OR: {
                int jumpOverRight = emitJumpIfTrue();
                emitByte(OP_POP);
                parsePrecedence(PREC_OR);
                patchJump(jumpOverRight);
                break;
            }
            case TOKEN_QUESTION: {
                if (precedence > PREC_TERNARY) return;
                advance();

                int jumpOverThen = emitJumpIfFalse();
                emitByte(OP_POP);  // condition
                expression();  // if true
                consume(TOKEN_COLON, "Expect ':' after '?' expression.");

                int jumpOverElse = emitJumpUnconditionally();

                patchJump(jumpOverThen);
                emitByte(OP_POP);  // condition
                expression();  // if false

                patchJump(jumpOverElse);
                return;
            }
            case TOKEN_LEFT_PAREN: {  // function call
                if (precedence > PREC_CALL) return;
                advance();
                int args = argumentList();
                emitBytes(OP_CALL, args);
                break;
            }
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


void Compiler::emitConstantAccess(Value value){
    // just in-case I call this by accident instead of writing the opcode myself.
    if (IS_BOOL(value)) AS_BOOL(value) ? emitByte(OP_TRUE) : emitByte(OP_FALSE);
    else if (IS_NIL(value)) emitByte(OP_NIL);
    else {
        const_index_t location = currentChunk()->addConstant(value);
        emitBytes(OP_GET_CONSTANT, location);
    }
}

int Compiler::parseLocalVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);


    // TODO: keep the name when in debug mode
    // identifierConstant(previous);
    return declareLocalVariable();
}

// for preventing self reference in a definition
int Compiler::declareLocalVariable(){
    if (locals()->count >= 256) {  // TODO: another opcode for more but that means i have to grow the stack as well
        errorAt(previous, "Too many local variables in function.");
        return -1;
    }

    Local local;
    local.depth = -1;
    local.name = previous;
    local.assignments = 0;
    local.isFinal = false;

    for (int i= (int) locals()->count - 1; i >= 0; i--){
        Local check = locals()->get(i);
        // since <variableStack> is a stack, the first timeMS you see something of a higher scope, everything will be
        if (check.depth != -1 && check.depth < scopeDepth()) break;
        if (identifiersEqual(check.name, local.name)) {
            errorAt(previous, "Already a variable with this name in this scope.");
        }
    }

    locals()->push(local);

    return (int) locals()->count - 1;
}

void Compiler::defineLocalVariable(){
    // Mark initialized
    (*locals()->peekLast()).depth = scopeDepth();
    // NO-OP at runtime, it just flows over and the stack slot becomes a local
}

int Compiler::resolveLocal(Token name){
    for (int i= (int) locals()->count - 1; i >= 0; i--){
        Local check = locals()->get(i);
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
    return currentChunk()->addConstant(varName);
}

void Compiler::namedVariable(Token name, bool canAssign) {
    // TODO: need another opcode if i grow the stack and allow >256 variableStack
    int local = resolveLocal(name);

    if (local == -1){
        errorAt(previous, "Undeclared variable.");
        return;
    }

    // If the variable is inside a high precedence expression, it has to be a get not a set.
    // Like a * b = c should be a syntax error not parse as a * (b = c).
    if (canAssign && match(TOKEN_EQUAL)){
        Local* variable = locals()->peek(local);
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

void Compiler::functionExpression(FunctionType funcType, ObjString* name){
    pushFunction(funcType);
    ObjFunction* func = functionStack.peekLast()->function;
    func->name = name;
    
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    if (!check(TOKEN_RIGHT_PAREN)){
        do {
            func->arity++;
            if (func->arity > 255) {
                errorAt(current, "Can't have more than 255 parameters.");
            }
            parseLocalVariable("Expect parameter name.");
            defineLocalVariable();
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    statement();  // not block(). allow one-liner functions like: fun add(a, b) return a + b;

    // scope not closed. return implicitly pops everything
    emitBytes(OP_NIL, OP_RETURN);

#ifdef COMPILER_DEBUG_PRINT_CODE
    debugger.setChunk(currentChunk());
    debugger.debug((char*) func->name->array.contents);
#endif

    popFunction();
    emitConstantAccess(OBJ_VAL(func));
}

int Compiler::argumentList(){
    int args = 0;
    if (!check(TOKEN_RIGHT_PAREN)){
        do {
            expression();
            args++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");
    if (args == 255) {
        errorAt(previous, "Can't have more than 255 arguments.");
    }

    return args;
}