#include "vm.h"
#include "compiler.h"
#include "common.h"
#include <cmath>

#define FORMAT_RUNTIME_ERROR(format, ...)     \
        fprintf(stderr, format, __VA_ARGS__); \
        runtimeError("");


#ifdef VM_PROFILING
long VM::instructionTimeTotal[256] = {};
int VM::instructionCount[256] = {};
#endif

VM::VM() {
    resetStack();
    objects = nullptr;
    compiler = Compiler();
    globals = Table();
    strings = Set();
    tempSavedChunk = nullptr;

    out = &cout;
    err = &cerr;
}

VM::~VM() {
    freeObjects();
    delete tempSavedChunk;
}

void VM::setChunk(Chunk* chunkIn){
    chunk = chunkIn;
    ip = chunk->getCodePtr();
    resetStack();

    #ifdef VM_DEBUG_TRACE_EXECUTION
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
    #ifdef VM_SAFE_MODE
    if (stackHeight() >= STACK_MAX){
        runtimeError("Stack overflow on push.");
        return;
    }
    #endif

    *stackTop = value;
    stackTop++;
}

inline Value VM::pop(){
    stackTop--;
    return *stackTop;
}

inline int VM::stackHeight(){
    return (int) (stackTop - stack);
}

InterpretResult VM::run() {
    #define READ_BYTE() (*(ip++))
    #define READ_CONSTANT() chunk->getConstant(READ_BYTE())
    #define READ_STRING() (AS_STRING(READ_CONSTANT()))
    #define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
    #ifdef VM_SAFE_MODE
    #define ASSERT_POP(count)                       \
                if (count > stackHeight()){         \
                    FORMAT_RUNTIME_ERROR("Cannot pop %d from stack of size %d.", count, stackHeight()) \
                    return INTERPRET_RUNTIME_ERROR; \
                }
    #else
    #define ASSERT_POP(count)
    #endif

    #define ASSERT_NUMBER(value, message)                   \
            if (!IS_NUMBER(value)){                         \
                runtimeError(message);                      \
                return INTERPRET_RUNTIME_ERROR;             \
            }

    #define ASSERT_SEQUENCE(value, message)                 \
            if (!IS_STRING(value)){                         \
                runtimeError(message);                      \
                return INTERPRET_RUNTIME_ERROR;             \
            }

    #define BINARY_OP(op_code, c_op, resultCast) \
            case op_code: {                      \
                 ASSERT_POP(2)                   \
                 ASSERT_NUMBER(peek(0), "Right operand to '" #c_op "' must be a number.")           \
                 ASSERT_NUMBER(peek(1), string("Left operand to '" #c_op "' must be a number."))    \
                 Value right = pop();            \
                 Value left = pop();             \
                 push(resultCast(AS_NUMBER(left) c_op AS_NUMBER(right)));                           \
                 break;                          \
            }

    for (;;){
        #ifdef VM_DEBUG_TRACE_EXECUTION
        if (!Debugger::silent) printValueArray(stack, stackTop);
        int instructionOffset = (int) (ip - chunk->getCodePtr());
        debug.debugInstruction(instructionOffset);
        #endif

        #ifdef VM_PROFILING
        auto loopStart = std::chrono::high_resolution_clock::now();
        #endif

        byte instruction;
        switch (instruction = READ_BYTE()) {
            case OP_ADD:
                ASSERT_POP(2)
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
            case OP_GET_LOCAL: {
                char offset = READ_BYTE();
                if (offset >= stackHeight()) {
                    FORMAT_RUNTIME_ERROR("Cannot get index %d of size %d. The bytecode must be invalid.", offset, stackHeight());
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(stack[offset]);
                break;
            }
            case OP_SET_LOCAL: {
                char offset = READ_BYTE();
                if (offset >= stackHeight()) {
                    FORMAT_RUNTIME_ERROR("Cannot set index %d of size %d. The bytecode must be invalid.", offset, stackHeight());
                    return INTERPRET_RUNTIME_ERROR;
                }
                stack[offset] = peek(0);
                break;
            }
            case OP_DEFINE_GLOBAL: {
                ASSERT_POP(1)
                // TODO: This doesn't check for redefining, but I'll do that as a compiler pass.
                ObjString* name = READ_STRING();
                globals.set(name, peek(0));
                pop();  // don't pop before table set so garbage collector can find if runs during that resize.
                break;
            }
            case OP_SET_GLOBAL: {
                ASSERT_POP(1)
                ObjString* name = READ_STRING();
                if (globals.set(name, peek(0))){
                    globals.remove(name);
                    FORMAT_RUNTIME_ERROR("Undefined variable '%s'.", asCString(name));
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
                    FORMAT_RUNTIME_ERROR("Undefined variable '%s'.", asCString(name));
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_ACCESS_INDEX: {
                ASSERT_POP(2)
                ASSERT_NUMBER(peek(0), "Array index must be an integer.")
                ASSERT_SEQUENCE(peek(1), "Slice target must be a sequence")
                int index = AS_NUMBER(pop());
                Value array = pop();
                Value result;
                bool success = accessSequenceIndex(array, index, &result);
                if (success) push(result);
                else return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_SLICE_INDEX: {
                ASSERT_POP(3)
                ASSERT_NUMBER(peek(0), "Slice end index must be an integer.")
                ASSERT_NUMBER(peek(1), "Slice start index must be an integer.")
                ASSERT_SEQUENCE(peek(2), "Slice target must be a sequence")
                int endIndex = AS_NUMBER(pop());
                int startIndex = AS_NUMBER(pop());
                Value array = pop();
                Value result;
                bool success = accessSequenceSlice(array, startIndex, endIndex, &result);
                if (success) push(result);
                else return INTERPRET_RUNTIME_ERROR;
                break;
            }
            case OP_GET_LENGTH: {
                ASSERT_POP(1)
                int stackOffset = READ_BYTE();
                ASSERT_SEQUENCE(peek(stackOffset), "Length target must be a sequence")
                Value array = peek(stackOffset);
                double result = getSequenceLength(array);
                if (result == -1) return INTERPRET_RUNTIME_ERROR;
                else push(NUMBER_VAL(result));
                break;
            }
            case OP_EXPONENT: {
                ASSERT_POP(2)
                ASSERT_NUMBER(peek(0), "Right operand to '**' must be a number.")
                ASSERT_NUMBER(peek(1), "Left operand to '**' must be a number.")
                Value right = pop();
                Value left = pop();
                push(NUMBER_VAL(pow(AS_NUMBER(left), AS_NUMBER(right))));
                break;
            }
            case OP_NEGATE:
                ASSERT_NUMBER(peek(0), "Operand to '-' must be a number.")
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_PRINT:
                ASSERT_POP(1)
                printValue(pop(), out);
                *out << endl;  // TODO: have a way to print without forcing the new line but should still generally push that for convince.
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
                ASSERT_POP(1)
                pop();
                break;
            case OP_POP_MANY: {
                int count = READ_BYTE();
                ASSERT_POP(count)
                stackTop -= count;
                break;
            }
            case OP_JUMP_IF_FALSE: {
                uint16_t distance = READ_SHORT();
                if (isFalsy(peek(0))) ip += distance;
                break;
            }
            case OP_JUMP: {
                uint16_t distance = READ_SHORT();
                ip += distance;
                break;
            }
            case OP_LOOP: {
                uint16_t distance = READ_SHORT();
                ip -= distance;
                break;
            }
            case OP_EXIT_VM:  // used to exit the repl or return from debugger.
                return INTERPRET_EXIT;

            case OP_LOAD_INLINE_CONSTANT:
                loadInlineConstant();
                break;
            #ifdef VM_ALLOW_DEBUG_BREAK_POINT
            case OP_DEBUG_BREAK_POINT:
                return INTERPRET_DEBUG_BREAK_POINT;
            #endif

            #ifdef VM_SAFE_MODE
            default:
                FORMAT_RUNTIME_ERROR("Unrecognised opcode '%d'. Index in chunk: %ld. Size of chunk: %d ", instruction, ip - chunk->getCodePtr() - 1, chunk->getCodeSize());
                return INTERPRET_RUNTIME_ERROR;
            #endif
        }

        #ifdef VM_PROFILING
        auto loopEnd = std::chrono::high_resolution_clock::now();
        instructionTimeTotal[instruction] += std::chrono::duration_cast<std::chrono::nanoseconds>(loopEnd - loopStart).count();
        instructionCount[instruction]++;
        #endif
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
    #undef ASSERT_NUMBER
    #undef BINARY_OP
    #undef READ_STRING
    #undef ASSERT_SEQUENCE
    #undef ASSERT_POP
    #undef READ_SHORT
}

void VM::runtimeError(const string& message){
    *err << message << endl;
    runtimeError();
}

void VM::runtimeError(){
    int instructionOffset = (int) (ip - chunk->getCodePtr() - 1);
    int line = chunk->getLineNumber(instructionOffset);
    *err << "[line " << line << "] in script" << endl;
}

bool VM::isFalsy(Value value){
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value)) || (IS_NUMBER(value) && AS_NUMBER(value) == 0);
}

void VM::concatenate(){
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());

    uint32_t length = a->array.length-1 + b->array.length-1;
    char* chars = ALLOCATE(char, length + 1);
    memcpy(chars, a->array.contents, a->array.length - 1);
    memcpy(chars + a->array.length - 1, b->array.contents, b->array.length - 1);
    chars[length] = '\0';

    ObjString* result = takeString(&strings, &objects, chars,  length);
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
    *out << "Current Chunk Constants:" << endl;
    chunk->printConstantsArray();
    *out << "Allocated Heap Objects:" << endl;
    printObjectsList(objects);
    *out << "Global Variables:";
    globals.printContents();
    *out << "Current Stack:" << endl;
    printValueArray(stack, stackTop - 1);
    *out << "Index in chunk: " << (ip - chunk->getCodePtr() - 1) << ". Length of chunk: " << chunk->getCodeSize()  << "." << endl;
}

void VM::loadInlineConstant(){
    unsigned char type = *ip;
    ip++;
    switch (type) {
        case 0: {
            double value;
            assert(sizeof(value) == 8);
            memcpy(&value, ip, sizeof(value));
            ip += sizeof(value);
            chunk->rawAddConstant(NUMBER_VAL(value));
            break;
        }
        case (1 + OBJ_STRING): {
            int length;
            assert(sizeof(length) == 4);
            memcpy(&length, ip, sizeof(length));
            ip += sizeof(length);
            ObjString* str = copyString(&strings, &objects, (char*) ip, length-1);
            chunk->rawAddConstant(OBJ_VAL(str));
            ip += length;
            break;
        }
        default:
            FORMAT_RUNTIME_ERROR("Invalid Value Type '%d'", type);
            break;
    }
}

bool VM::accessSequenceIndex(Value array, int index, Value* result){
    if (IS_STRING(array)){
        ObjString* str = AS_STRING(array);
        uint32_t realIndex = index < 0 ? str->array.length - 1 + index : index;
        if (realIndex >= str->array.length-1){
            FORMAT_RUNTIME_ERROR("Index '%d' out of bounds for string '%s'.", index, AS_CSTRING(array));
            return INTERPRET_RUNTIME_ERROR;
        }
        *result = OBJ_VAL(copyString(&strings, &objects, AS_CSTRING(array) + realIndex, 1));
        return true;
    } else {
        runtimeError("Unrecognised sequence type");
        return false;
    }
}

bool VM::accessSequenceSlice(Value array, int startIndex, int endIndex, Value* result){
    if (IS_STRING(array)){
        ObjString* str = AS_STRING(array);
        uint32_t realEndIndex = endIndex < 0 ? str->array.length - 1 + endIndex : endIndex;
        uint32_t realStartIndex = startIndex < 0 ? str->array.length - 1 + startIndex : startIndex;
        if (realEndIndex > str->array.length - 1){
            FORMAT_RUNTIME_ERROR("Index '%u' out of bounds for string '%s'.", endIndex, AS_CSTRING(array));
            return INTERPRET_RUNTIME_ERROR;
        }
        if (realStartIndex >= str->array.length - 1){
            FORMAT_RUNTIME_ERROR("Index '%u' out of bounds for string '%s'.", startIndex, AS_CSTRING(array));
            return INTERPRET_RUNTIME_ERROR;
        }
        if (realEndIndex <= realStartIndex) {
            FORMAT_RUNTIME_ERROR("Invalid sequence slice. Start: '%d' (inclusive), End: '%d' (exclusive).", realStartIndex, realEndIndex);
            return INTERPRET_RUNTIME_ERROR;
        }

        ObjString* newString = copyString(&strings, &objects, AS_CSTRING(array) + realStartIndex, (int) (realEndIndex - realStartIndex));
        *result = OBJ_VAL(newString);
        return true;
    } else {
        runtimeError("Unrecognised sequence type");
        return false;
    }
}

double VM::getSequenceLength(Value array){
    if (IS_STRING(array)){
        ObjString* str = AS_STRING(array);
        return str->array.length - 1;
    } else {
        runtimeError("Unrecognised sequence type");
        return -1;
    }
}

void VM::printTimeByInstruction(){
    #ifdef VM_PROFILING
        cout << "VM Time per Instruction Type" << endl;
        long totalTime = 0;
        long totalLoops = 0;
        for (int i=0;i<256;i++){
            totalTime += instructionTimeTotal[i];
            totalLoops += instructionCount[i];
        }
        printf("%25s: %10ld ns (%.2f%%) for %7ld times (%.2f%%)\n","total", totalTime, 100.0, totalLoops, 100.0);

        for (int i=0;i<256;i++){
            if (instructionCount[i] > 0) {
                double percentTime = (double) instructionTimeTotal[i] / (double) totalTime * 100;
                double percentLoops = (double) instructionCount[i] / (double) totalLoops * 100;
                printf("%25s: %10ld ns (%5.1f%%) for %7d times (%.2f%%)\n", Chunk::opcodeNames[i].c_str(), instructionTimeTotal[i], percentTime, instructionCount[i], percentLoops);
            }
        }
    #endif
}

#undef FORMAT_RUNTIME_ERROR