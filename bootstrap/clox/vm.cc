#include "vm.h"
#include "compiler.h"
#include <cmath>

VM::VM() {
    resetStack();
    objects = nullptr;
    compiler = new Compiler(&objects);
    #ifdef DEBUG_TRACE_EXECUTION
    debug = new Debugger();
    #endif
}

VM::~VM() {
    freeObjects();
    delete compiler;
    #ifdef DEBUG_TRACE_EXECUTION
    delete debug;
    #endif
}

void VM::setChunk(Chunk* chunk){
    this->chunk = chunk;
    ip = chunk->code->data;
    compiler->setChunk(chunk);

    #ifdef DEBUG_TRACE_EXECUTION
    debug->setChunk(chunk);
    #endif
}

void VM::resetStack(){
    stackTop = stack;
}

InterpretResult VM::interpret(Chunk* chunk) {
    setChunk(chunk);
    return run();
}

InterpretResult VM::interpret(char* src) {
    Chunk* bytecode = new Chunk();
    setChunk(bytecode);  // set the internal compiler to point to the new chunk

    InterpretResult result;
    if (compiler->compile(src)){
        setChunk(bytecode);  // set the instruction pointer to the start of the newly compiled array
        run();
        result = INTERPRET_OK;
    } else {
        result = INTERPRET_COMPILE_ERROR;
    }

    delete bytecode;
    return result;
}

void VM::push(Value value){
    *stackTop = value;
    stackTop++;
}

Value VM::pop(){
    stackTop--;
    return *stackTop;
}

Value VM::peek(int distance) {
    return stackTop[-1 - distance];
}

InterpretResult VM::run() {
    #define READ_BYTE() (*(ip++))
    #define READ_CONSTANT() chunk->constants->get(READ_BYTE())
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

    for (;;){
        #ifdef DEBUG_TRACE_EXECUTION
        cout << "          ";
        for (Value* slot = stack; slot < stackTop; slot++){
            cout << "[";
            printValue(*slot);
            cout << "]";
        }
        cout << endl;

        debug->debugInstruction((int) (ip - chunk->code->data));
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

                }
                break;
            BINARY_OP(OP_SUBTRACT, -, NUMBER_VAL)
            BINARY_OP(OP_MULTIPLY, *, NUMBER_VAL)
            BINARY_OP(OP_DIVIDE, /, NUMBER_VAL)
            BINARY_OP(OP_GREATER, >, BOOL_VAL)
            BINARY_OP(OP_LESS, <, BOOL_VAL)
            case OP_CONSTANT:
                push(READ_CONSTANT());
                break;
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
            case OP_RETURN:
                printValue(pop());
                cout << endl;
                return INTERPRET_OK;
            case OP_NIL:
                push(NIL_VAL);
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
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef ASSERT_NUMBER
    #undef BINARY_OP
}

void VM::runtimeError(const string& message){
    cerr << message << endl;
    int instruction = (int) (ip - chunk->code->data - 1);
    int line = chunk->lines->get(instruction);
    cerr << "[line " << line << "] in script" << endl;
}

bool VM::isFalsy(Value value){
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)) || (IS_NUMBER(value) && AS_NUMBER(value) == 0);
}

bool VM::valuesEqual(Value right, Value left) {
    if (right.type != left.type) return false;

    // Book: cant just memcmp() the structs cause there's padding so garbage bites
    switch (right.type) {
        case VAL_BOOL:
            return AS_BOOL(right) == AS_BOOL(left);
        case VAL_NUMBER:
            return AS_NUMBER(right) == AS_NUMBER(left);
        case VAL_NIL:
            return true;
        case VAL_OBJ: {
            ObjString* a = AS_STRING(left);
            ObjString* b = AS_STRING(right);
            return a->length == b->length && memcmp(a->chars, b->chars, a->length) == 0;
        }
        default:
            return false;
    }
}

void VM::concatenate(){
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    int length = a->length + b->length;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->chars, a->length);
    memcpy(chars + a->length, b->chars, b->length);
    chars[length] = '\0';

    ObjString* result = takeString(&objects, chars, length);
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

void VM::freeObject(Obj* object){
    switch (object->type) {
        case OBJ_STRING: {
            cout << "free: ";
            printValue(OBJ_VAL(object));
            cout << endl;

            // We own the char array.
            ObjString* string = (ObjString*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(ObjString, object);
            break;
        }

    }
}
