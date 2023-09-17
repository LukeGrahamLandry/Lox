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

void Compiler::unary(){
    TokenType operatorType = previous.type;
    parsePrecedence(PREC_UNARY);  // vm: push value to stack

    switch (operatorType) {
        case TOKEN_MINUS:
            emitByte(OP_NEGATE);  // vm: pop it off, negate, put back
            break;
        case TOKEN_BANG:
            emitByte(OP_NOT);
            break;
        default:
            cerr << "Unreachable unary token." << endl;
            std::abort();
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
    return OBJ_VAL(gc.copyString(chars, length));
}

void Compiler::parsePrecedence(Precedence precedence){
    advance();

#define LITERAL(token, opcode)   \
            case token:              \
                emitByte(opcode);    \
                break;

    bool canAssign = precedence <= PREC_ASSIGNMENT;

    switch (previous.type) {
        case TOKEN_NUMBER: number(); break;
        case TOKEN_STRING: string(); break;
        case TOKEN_LEFT_PAREN: grouping(); break;
        case TOKEN_MINUS:  // fallthrough
        case TOKEN_BANG:
            unary();
            break;

        LITERAL(TOKEN_TRUE, OP_TRUE)
        LITERAL(TOKEN_FALSE, OP_FALSE)
        LITERAL(TOKEN_NIL, OP_NIL)
        case TOKEN_IDENTIFIER:
            namedVariable(previous, canAssign);
            break;
        case TOKEN_FUN: {
            if (check(TOKEN_IDENTIFIER)) break;
            std::string n = "lambda:" + to_string(previous.line);
            ObjString* name = gc.copyString(n.c_str(), (int) n.length());
            functionExpression(TYPE_FUNCTION, name);
            break;
        }
        case TOKEN_THIS:
            namedVariable(previous, false);
            break;
        case TOKEN_SUPER: {
            superAccess();
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
                advance();
                int jumpOverRight = emitJumpIfFalse();
                emitByte(OP_POP);
                parsePrecedence(PREC_AND);
                patchJump(jumpOverRight);
                break;
            }
            case TOKEN_OR: {
                advance();
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
            case TOKEN_DOT: {  // field access
                if (precedence > PREC_CALL) return;
                advance();
                consume(TOKEN_IDENTIFIER, "Expect property name after '.'.");
                int nameId = identifierConstant(previous);

                if (canAssign && match(TOKEN_EQUAL)) {
                    expression();
                    emitBytes(OP_SET_PROPERTY, nameId);
                } else if (match(TOKEN_LEFT_PAREN)) {  // Fast path for direct method call
                    int args = argumentList();
                    emitBytes(OP_INVOKE, nameId);
                    emitByte(args);
                } else {
                    emitBytes(OP_GET_PROPERTY, nameId);
                }
                break;
            }
            default:
                return;
        }
    }

#undef BINARY_INFIX_OP
#undef LITERAL
#undef DOUBLE_BINARY_INFIX_OP
}

Token Compiler::syntheticToken(const char* name) {
    Token t;
    t.start = name;
    t.length = strlen(name);
    return t;
}

void Compiler::superAccess() {
    // TODO: different message if not in a class at all.
    if (!currentHasSuper) {
        errorAt(previous, "Can't use 'super' in a class with no superclass.");
    }

    consume(TOKEN_DOT, "Expect '.' after 'super'.");
    consume(TOKEN_IDENTIFIER, "Expect superclass method name.");
    int methodNameId = identifierConstant(previous);
    namedVariable(syntheticToken("this"), false);
    if (match(TOKEN_LEFT_PAREN)) {  // Fast path for direct method call
        int args = argumentList();
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_SUPER_INVOKE, methodNameId);
        emitByte(args);
    } else {
        namedVariable(syntheticToken("super"), false);
        emitBytes(OP_GET_SUPER, methodNameId);
    }
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
        const_index_t location = currentChunk()->addConstant(value, gc);
        emitBytes(OP_GET_CONSTANT, location);
    }
}

int Compiler::parseLocalVariable(const char* errorMessage) {
    consume(TOKEN_IDENTIFIER, errorMessage);


    // TODO: keep the name when in debug mode
    // identifierConstant(previous);
    return declareLocalVariable();
}

// Must call defineLocalVariable after.
// They're separate for preventing self reference in a definition
int Compiler::makeLocal(Token name) {
    if (getLocals().count >= 256) {  // TODO: another opcode for more but that means i have to grow the stack as well
        errorAt(previous, "Too many local variables in function.");
        return -1;
    }

    Local local;
    local.depth = -1;
    local.name = name;
    local.assignments = 0;
    local.isFinal = false;
    local.isCaptured = false;

    for (int i= (int) getLocals().count - 1; i >= 0; i--){
        Local check = getLocals()[i];
        // since <variableStack> is a stack, the first timeMS you see something of a higher scope, everything will be
        if (check.depth != -1 && check.depth < scopeDepth()) break;
        if (identifiersEqual(check.name, local.name)) {
            errorAt(previous, "Already a variable with this name in this scope.");
        }
    }

    getLocals().push(local, gc);

    return (int) getLocals().count - 1;
}

int Compiler::declareLocalVariable(){
    return makeLocal(previous);
}

void Compiler::defineLocalVariable(){
    // Mark initialized
    getLocals().peekLast().depth = scopeDepth();
    // NO-OP at runtime, it just flows over and the stack slot becomes a local
}

int Compiler::resolveLocal(TargetFunction& func, Token name){
    ArrayList<Local>& locals = *func.variableStack;
    for (int i= (int) locals.count - 1; i >= 0; i--){
        Local check = locals[i];
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
    return currentChunk()->addConstant(varName, gc);
}

void Compiler::namedVariable(Token name, bool canAssign) {
    // TODO: need another opcode if i grow the stack and allow >256 variableStack
    int local = resolveLocal(functionStack.peekLast(), name);

    OpCode set_op = OP_SET_LOCAL;
    OpCode get_op = OP_GET_LOCAL;

    if (local == -1) {
        int upi = resolveUpvalue(functionStack.count - 1, &name);
        if (upi != -1) {
            set_op = OP_SET_UPVALUE;
            get_op = OP_GET_UPVALUE;
            local = upi;
        } else {
            errorAt(previous, "Undeclared variable.");
            return;
        }
    }

    // If the variable is inside a high precedence expression, it has to be a get not a set.
    // Like a * b = c should be a syntax error not parse as a * (b = c).
    if (canAssign && match(TOKEN_EQUAL)){
        Local& variable = getLocals().peek(local);
        if (variable.assignments++ != 0 && variable.isFinal) {
            errorAt(previous, "Cannot assign to final variable.");
            return;
        }
        expression();
        emitBytes(set_op, local);
    } else {
        emitBytes(get_op, local);
    }
}

int Compiler::resolveUpvalue(int funcIndex, Token* name) {
    // in the main script and didn't find it.
    if (funcIndex == 0) return -1;

    auto currentFunc = &functionStack.data[funcIndex];
    int local = resolveLocal(functionStack[funcIndex - 1], *name);
    if (local != -1){
        (*functionStack[funcIndex - 1].variableStack)[local].isCaptured = true;
        return addUpvalue(*currentFunc, (uint8_t)local, true);
    }

    int upvalue = resolveUpvalue(funcIndex - 1, name);
    if (upvalue != -1) {
        return addUpvalue(*currentFunc, (uint8_t)upvalue, false);
    }

    // got to the main script without finding it.
    return -1;
}

int Compiler::addUpvalue(TargetFunction& func, uint8_t index, bool isLocal) {
    for (int i=0;i<func.upvalues->count;i++) {
        Upvalue val = (*func.upvalues)[i];
        if (val.index == index && val.isLocal == isLocal) {
            return i;
        }
    }

    Upvalue val = { index, isLocal };
    func.upvalues->push(val, gc);
    if (func.function->upvalueCount >= 255) {  // TODO
        cerr << "Too many up values. Need bigger index!" << endl;
        exit(65);
    }
    return func.function->upvalueCount++;
}

void Compiler::functionExpression(FunctionType funcType, ObjString* name){
    pushFunction(funcType);
    ObjFunction* func = functionStack.peekLast().function;
    func->name = name;

    Token t = previous;
    beginScope();
    consume(TOKEN_LEFT_PAREN, "Expect '(' after function name.");

    if (!check(TOKEN_RIGHT_PAREN)){
        do {
            if (func->arity >= 255) {
                errorAt(current, "Can't have more than 255 parameters.");
            }
            parseLocalVariable("Expect parameter name.");
            defineLocalVariable();
            func->arity++;
        } while (match(TOKEN_COMMA));
    }

    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    consume(TOKEN_LEFT_BRACE, "Expect '{' before function body.");
    block();

    // scope not closed. return implicitly pops everything
    emitEmptyReturn();

#ifdef COMPILER_DEBUG_PRINT_CODE
    debugger.setChunk(currentChunk());
    debugger.debug((char*) func->name->array.contents);
#endif

    TargetFunction target = functionStack.pop();

    const_index_t location = currentChunk()->addConstant(OBJ_VAL(func), gc);
    emitBytes(OP_CLOSURE, location);

    if (target.upvalues->count != target.function->upvalueCount) {
        errorAt(t, "ICE. Incorrect upvalue count");
    }

    for (int i=0;i<func->upvalueCount;i++) {
        Upvalue val = (*target.upvalues)[i];
        emitByte(val.isLocal ? 1 : 0);
        emitByte(val.index);
    }
}

// a, b, c)
int Compiler::argumentList(){
    int args = 0;
    if (!check(TOKEN_RIGHT_PAREN)){
        do {
            if (args >= 255) {
                errorAt(current, "Can't have more than 255 arguments.");
            }
            expression();
            args++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RIGHT_PAREN, "Expect ')' after parameters.");

    return args;
}