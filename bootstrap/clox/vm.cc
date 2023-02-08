#include "vm.h"
#include "compiler.h"
#include "common.h"
#include <cmath>

#define FORMAT_RUNTIME_ERROR(format, ...)     \
        fprintf(stderr, format, __VA_ARGS__); \
        runtimeError();

VM::VM() {
    resetStack();
    objects = nullptr;
    compiler = Compiler();
    globals = Table();
    strings = Set();
    tempSavedChunk = nullptr;
}

VM::~VM() {
    freeObjects();
    delete tempSavedChunk;
}

void VM::setChunk(Chunk* chunkIn){
    chunk = chunkIn;
    ip = chunk->getCodePtr();

    #ifdef DEBUG_TRACE_EXECUTION
    debug.setChunk(chunk);
    #endif
}

void VM::resetStack(){
    stackTop = stack;
}

InterpretResult VM::interpret(Chunk* chunk) {
    setChunk(chunk);
    return run();
}

bool VM::loadFromSource(char* src) {
    if (tempSavedChunk != nullptr) {
        delete tempSavedChunk;
        chunk = nullptr;
        tempSavedChunk = nullptr;
    }

    tempSavedChunk = new Chunk();

    InterpretResult result;
    compiler.strings = &strings;
    compiler.setChunk(tempSavedChunk);
    if (compiler.compile(src)){
        setChunk(tempSavedChunk);  // set the instruction pointer to the start of the newly compiled array

        if (compiler.objects != nullptr){
            linkObjects(&objects, compiler.objects);
            compiler.objects = nullptr;
        }
        // strings.safeAddAll(compiler.strings);
        // compiler.strings.removeAll();

        return true;
    } else {
        delete tempSavedChunk;
        tempSavedChunk = nullptr;
        return false;
    }
}

inline void VM::push(Value value){
    *stackTop = value;
    stackTop++;
}

inline Value VM::pop(){
    // TODO: take out this check eventually cause it probably slows it down a bunch and will never happen for valid bytecode.
    if (stackTop == stack){
        runtimeError("pop called on empty stack. The bytecode must be invalid. ");
        return NIL_VAL();
    }

    stackTop--;
    return *stackTop;
}

inline Value VM::peek(int distance) {
    return stackTop[-1 - distance];
}

InterpretResult VM::run() {
    #define READ_BYTE() (*(ip++))
    #define READ_CONSTANT() chunk->getConstant(READ_BYTE())
    #define READ_STRING() (AS_STRING(READ_CONSTANT()))
    #define ASSERT_NUMBER(value)                            \
            if (!IS_NUMBER(value)){                         \
                runtimeError("Operand must be a number.");  \
                return INTERPRET_RUNTIME_ERROR;             \
            }

    #define BINARY_OP(op_code, c_op, resultCast) \
            case op_code: {               \
                 ASSERT_NUMBER(peek(0))   \
                 ASSERT_NUMBER(peek(1))   \
                 Value right = pop();     \
                 Value left = pop();      \
                 push(resultCast(AS_NUMBER(left) c_op AS_NUMBER(right))); \
                 break;                   \
            }

    #ifdef DEBUG_TRACE_EXECUTION
    printDebugInfo();
    #endif

    for (;;){
        #ifdef DEBUG_TRACE_EXECUTION
        printValueArray(stack, stackTop);
        int index = (int) (ip - chunk->getCodePtr());
        debug.debugInstruction(index);
        #endif

        uint8_t instruction;
        switch (instruction = READ_BYTE()) {
            case OP_ADD:
                if (IS_STRING(peek(0)) && IS_STRING(peek(1))){
                    concatenate();
                } else if (IS_NUMBER(peek(0)) && IS_NUMBER(peek(1))){
                    Value right = pop();
                    Value left = pop();
                    push(NUMBER_VAL(AS_NUMBER(left) + AS_NUMBER(right)));
                } else {
                    runtimeError("Operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            BINARY_OP(OP_SUBTRACT, -, NUMBER_VAL)
            BINARY_OP(OP_MULTIPLY, *, NUMBER_VAL)
            BINARY_OP(OP_DIVIDE, /, NUMBER_VAL)
            BINARY_OP(OP_GREATER, >, BOOL_VAL)
            BINARY_OP(OP_LESS, <, BOOL_VAL)
            case OP_GET_CONSTANT:
                push(READ_CONSTANT());
                break;
            case OP_DEFINE_GLOBAL: {
                // TODO: This doesn't check for redefining, but I'll do that as a compiler pass.
                ObjString* name = READ_STRING();
                globals.set(name, peek(0));
                pop();  // don't pop before table set so garbage collector can find if runs during that resize.
                break;
            }
            case OP_SET_GLOBAL: {
                ObjString* name = READ_STRING();
                if (globals.set(name, peek(0))){
                    globals.remove(name);
                    FORMAT_RUNTIME_ERROR("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }

                // Since an assignment is an expression, it shouldn't modify the stack, so they can chain.
                // If it was used as a statement, a separate OP_POP will be added automatically.
                // To avoid a special case we want to make sure that every valid expression adds exactly one thing to the stack.
                // pop();
                break;
            }
            case OP_GET_GLOBAL: {
                ObjString* name = READ_STRING();
                Value value = NIL_VAL();
                bool found = globals.get(name, &value);
                if (found){
                    push(value);
                } else {
                    FORMAT_RUNTIME_ERROR("Undefined variable '%s'.", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }

            case OP_EXPONENT: {
                ASSERT_NUMBER(peek(0))
                ASSERT_NUMBER(peek(1))
                Value right = pop();
                Value left = pop();
                push(NUMBER_VAL(pow(AS_NUMBER(left), AS_NUMBER(right))));
                break;
            }
            case OP_NEGATE:
                ASSERT_NUMBER(peek(0))
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_PRINT:
                printValue(pop());
                cout << endl;  // TODO: have a way to print without forcing the new line but should still generally add that for convince.
                break;
            case OP_RETURN:
                return INTERPRET_OK;
            case OP_NIL:
                push(NIL_VAL());
                break;
            case OP_TRUE:
                push(BOOL_VAL(true));
                break;
            case OP_FALSE:
                push(BOOL_VAL(false));
                break;
            case OP_NOT:
                push(BOOL_VAL(isFalsy(pop())));
                break;
            case OP_EQUAL:
                push(BOOL_VAL(valuesEqual(pop(), pop())));
                break;
            case OP_POP:
                if (stackTop == stack){
                    runtimeError("pop called on empty stack. The bytecode must be invalid. ");
                    return INTERPRET_RUNTIME_ERROR;
                }
                pop();
                break;

            case OP_EXIT_VM:  // used to exit the repl or return from debugger.
                return INTERPRET_EXIT;

            #ifdef ALLOW_DEBUG_BREAK_POINT
            case OP_DEBUG_BREAK_POINT:
                return INTERPRET_DEBUG_BREAK_POINT;
            #endif
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef ASSERT_NUMBER
    #undef BINARY_OP
    #undef READ_STRING
}

void VM::runtimeError(const string& message){
    cerr << message << endl;
    runtimeError();
}

void VM::runtimeError(){
    int instructionOffset = (int) (ip - chunk->getCodePtr() - 1);
    int line = chunk->getLineNumber(instructionOffset);
    cerr << "[line " << line << "] in script" << endl;
}

bool VM::isFalsy(Value value){
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)) || (IS_NUMBER(value) && AS_NUMBER(value) == 0);
}

void VM::concatenate(){
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(&strings, &objects, chars, length);
    push(OBJ_VAL(result));
}

void VM::freeObjects(){
    Obj* object = objects;
    while (object != nullptr) {
        Obj* next = object->next;
        freeObject(object);
        object = next;
    }
}

void VM::printDebugInfo() {
    cout << "Current Chunk Constants:" << endl;
    chunk->printConstantsArray();
    cout << "Allocated Heap Objects:" << endl;
    printObjectsList(objects);
    cout << "Global Variables:";
    globals.printContents();
    cout << "Location in chunk: " << (ip - chunk->getCodePtr()) << " / " << chunk->getCodeSize() << endl;
}

#undef FORMAT_RUNTIME_ERROR