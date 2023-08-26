#include "vm.h"
#include "compiler/compiler.h"
#include "common.h"
#include <cmath>
#include "natives.h"

#define FORMAT_RUNTIME_ERROR(format, ...)     \
        fprintf(stderr, format, __VA_ARGS__); \
        runtimeError("");


#ifdef VM_PROFILING
long VM::instructionTimeTotal[256] = {};
int VM::instructionCount[256] = {};
#endif

VM::VM() : compiler(Compiler(gc)) {
    evilVmGlobal = this;
    resetStack();
    gc.objects = nullptr;
    gc.natives = new Table(gc);
    gc.strings = new Set(gc);
    gc.frameCount = 0;

    out = &cout;
    err = &cerr;
    exitCode = 0;
    gc.openUpvalues = nullptr;
    gc.enable = false;

    defineNative("clock", LoxNatives::klock, 0);
    defineNative("time", LoxNatives::time, 0);
    defineNative("input", LoxNatives::input, 0);
    defineNative("eval", LoxNatives::eval, 1);
}

VM::~VM() {
    freeObjects();
    evilVmGlobal = nullptr;
}

void VM::resetStack(){
    gc.stackTop = gc.stack;
}

InterpretResult VM::interpret(char* src) {
    if (!loadFromSource(src)) return INTERPRET_COMPILE_ERROR;
    return run();
}

bool VM::loadFromSource(char* src) {
    gc.enable = false;
    ObjFunction* function = compiler.compile(src);
    if (function == nullptr) return false;

    push(OBJ_VAL(function));
    ObjClosure* closure = gc.newClosure(function);
    pop();
    push(OBJ_VAL(closure));
    if (function->upvalueCount != 0) {
        cerr << "ICE. Script has upvalues." << endl;
    }

    // this doesn't actually run it, just sets it as the current frame on the call stack
    call(closure, 0);

    gc.enable = true;
    return true;
}

Value VM::produceFunction(char* src) {
    ObjFunction* function = compiler.compile(src);
    if (function == nullptr) return NIL_VAL();
    return OBJ_VAL(function);
}

inline void VM::push(Value value){
    #ifdef VM_SAFE_MODE
    if (stackHeight() >= STACK_MAX){
        runtimeError("Stack overflow on push.");
        return;
    }
    #endif

    *gc.stackTop = value;
    gc.stackTop++;
}

inline Value VM::pop(){
    gc.stackTop--;
    return *gc.stackTop;
}

inline int VM::stackHeight(){
    return (int) (gc.stackTop - gc.stack);
}

InterpretResult VM::run() {
    CallFrame frame;
    Chunk* chunk;
    #define CACHE_FRAME()                             \
            frame = gc.frames[gc.frameCount - 1];           \
            chunk = frame.closure->function->chunk;            \
            ip = frame.ip;                            \

    #define READ_BYTE() (*(ip++))
    #define READ_CONSTANT() chunk->getConstant(READ_BYTE())
    #define READ_STRING() (AS_STRING(READ_CONSTANT()))
    #define READ_SHORT() (ip += 2, (uint16_t)((ip[-2] << 8) | ip[-1]))
    #define STACK_BASE() (frame.slots)

    #ifdef VM_SAFE_MODE
    // enough stack entries for: for (int i=0;i<count;i++)(pop());
    #define ASSERT_POP(count)                                \
                if (count > (int) (gc.stackTop - STACK_BASE())){ \
                    FORMAT_RUNTIME_ERROR("Cannot pop %d from stack frame of size %d.", count, (int) (gc.stackTop - frame.slots)) \
                    return INTERPRET_RUNTIME_ERROR;          \
                }
    // enough stack entries for: frame.slots[offset]
    #define ASSERT_PEEK(offset) ASSERT_POP(offset + 1)
    #else
    #define ASSERT_POP(count)
    #define ASSERT_PEEK(offset)
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
                 ASSERT_NUMBER(peek(0), "Operands must be numbers.")           \
                 ASSERT_NUMBER(peek(1), string("Operands must be numbers."))    \
                 Value right = pop();            \
                 Value left = pop();             \
                 push(resultCast(AS_NUMBER(left) c_op AS_NUMBER(right)));                           \
                 break;                          \
            }

    CACHE_FRAME()
    for (;;){
        #ifdef VM_DEBUG_TRACE_EXECUTION
        if (!Debugger::silent) {
            cout << frame.slots - gc.stack;
            printValueArray(gc.stack, gc.stackTop);
        }

        int instructionOffset = (int) (ip - frame.closure->function->chunk->getCodePtr());
        debug.setChunk(chunk);
        debug.debugInstruction(instructionOffset);
        // printDebugInfo();
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
                ASSERT_PEEK(offset)
                push(STACK_BASE()[offset]);
                break;
            }
            case OP_SET_LOCAL: {
                char offset = READ_BYTE();
                ASSERT_PEEK(offset)
                STACK_BASE()[offset] = peek(0);
                // Since an assignment is an expression, it shouldn't pop the stack, so they can chain.
                // If it was used as a statement, a separate OP_POP will be added automatically.
                // To avoid a special case we want to make sure that every valid expression adds exactly one thing to the stack.
                break;
            }
            case OP_ACCESS_INDEX: {
                ASSERT_POP(2)
                ASSERT_NUMBER(peek(0), "Array index must be an integer.")
                ASSERT_SEQUENCE(peek(1), "Slice target must be a sequence")
                int index = AS_NUMBER(pop());
                Value array = peek();  // dont pop here cause gc
                Value result;
                bool success = accessSequenceIndex(array, index, &result);
                pop();
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
                Value array = peek();  // dont pop here cause gc
                Value result;
                bool success = accessSequenceSlice(array, startIndex, endIndex, &result);
                pop();
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
                ASSERT_NUMBER(peek(0), "Operand must be a number.")
                push(NUMBER_VAL(-AS_NUMBER(pop())));
                break;
            case OP_PRINT:
                ASSERT_POP(1)
                printValue(pop(), out);
                *out << endl;  // TODO: have a way to print without forcing the new line but should still generally push that for convince.
                break;
            case OP_RETURN: {
                ASSERT_POP(1)
                Value value = pop();  // get the return value

                gc.frameCount--;
                if (gc.frameCount == 0) {
                    resetStack();
                    if (IS_NUMBER(value)) {
                        exitCode = (int) AS_NUMBER(value);
                        return INTERPRET_OK;
                    } else {
                        runtimeError("Top level return value must be an integer (for process exit code)");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }

                closeUpvalues(frame.slots);  // Check if any locals we're about to pop need to be promoted to upvalues.
                gc.stackTop = frame.slots;  // move the stack back to the first slot. pops the value that was called, any args passed and any function getLocals.
                CACHE_FRAME()  // point the ip back to the caller's code
                push(value);  // put the return value back on the stack
                break;
            }
            case OP_CLOSURE: {
                ASSERT_POP(1)
                ObjFunction* function = AS_FUNCTION(READ_CONSTANT());
                push(OBJ_VAL(function));  // for gc
                ObjClosure* closure = gc.newClosure(function);
                pop();
                push(OBJ_VAL(closure));

                // Growing the list here, not in newClosure so that the closure pointer is on the stack first incase gc triggers.
                closure->upvalues.growExact(function->upvalueCount, gc);
                for (int i=0;i<function->upvalueCount;i++) {
                    uint8_t isLocal = READ_BYTE();
                    uint8_t index = READ_BYTE();
                    if (isLocal) {
                        closure->upvalues.push(captureUpvalue(frame.slots + index), gc);
                    } else {
                        closure->upvalues.push(frame.closure->upvalues[index], gc);
                    }
                }
                break;
            }
            case OP_CLOSE_UPVALUE: {
                closeUpvalues(gc.stackTop - 1);
                pop();
                break;
            }
            case OP_GET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                push(*frame.closure->upvalues[slot]->location);
                break;
            }
            case OP_SET_UPVALUE: {
                uint8_t slot = READ_BYTE();
                *frame.closure->upvalues[slot]->location = peek(0);
                break;
            }
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
                gc.stackTop -= count;
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
            case OP_CALL: {
                int argCount = READ_BYTE();
                ASSERT_POP(argCount + 1)
                if (!callValue(peek(argCount), argCount)) {
                    return INTERPRET_RUNTIME_ERROR;
                }
                CACHE_FRAME()
                break;
            }
            case OP_EXIT_VM:  // used to exit the repl or return from debugger.
                return INTERPRET_EXIT;

            case OP_LOAD_INLINE_CONSTANT:
                loadInlineConstant();
                break;
            case OP_DEBUG_BREAK_POINT:
                printDebugInfo();
                break;

            #ifdef VM_SAFE_MODE
            default: {
                FORMAT_RUNTIME_ERROR("Unrecognised opcode '%d'. Index in chunk: %ld. Size of chunk: %d ", instruction, ip - currentChunk()->getCodePtr() - 1, currentChunk()->getCodeSize());
                return INTERPRET_RUNTIME_ERROR;
            }
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

ObjUpvalue* VM::captureUpvalue(Value* local){
#ifdef VM_DEBUG_TRACE_EXECUTION
    cout << "[Capture]: ";
    printValue(*local);
    cout << endl;
#endif

    ObjUpvalue* prev = nullptr;
    ObjUpvalue* val = gc.openUpvalues;
    while (val != nullptr && val->location > local) {
        prev = val;
        val = val->next;
    }

    if (val != nullptr && val->location == local) {
        return val;
    }

    ObjUpvalue* newVal = gc.newUpvalue(local);
    newVal->next = val;
    if (prev == nullptr) {
        gc.openUpvalues = newVal;
    } else {
        prev->next = newVal;
    }
    return newVal;
}

void VM::closeUpvalues(Value* last) {
    // The locations point to the stack so the pointer order is meaningful.
    // Go along open upvalues until you hit on earlier in the stack than <last>.
    while (gc.openUpvalues != nullptr && gc.openUpvalues->location >= last) {
#ifdef VM_DEBUG_TRACE_EXECUTION
        cout << "[Close] ";
        printValue(*gc.openUpvalues->location);
        cout << endl;
#endif
        // Steak the value and point to yourself instead of the stack.
        gc.openUpvalues->closed = *gc.openUpvalues->location;
        gc.openUpvalues->location = &gc.openUpvalues->closed;
        gc.openUpvalues = gc.openUpvalues->next;
    }
}

void VM::runtimeError(const string& message){
    *err << message << endl;
    printStackTrace(err);
}

void VM::printStackTrace(ostream* output){
    for (int i = gc.frameCount - 1; i >= 0; i--) {
        CallFrame* frame = &gc.frames[i];
        ObjFunction* function = frame->closure->function;
        int instructionOffset = (int) (frame->ip - function->chunk->getCodePtr() - 1);
        int line = function->chunk->getLineNumber(instructionOffset);
        *output << "[line " << line << "] in ";
        if (function->name == nullptr) {
            *output << "script" << endl;
        } else {
            *output << ((char*) function->name->array.contents) << endl;
        }
    }
}

bool VM::isFalsy(Value value){
    // The book says zero is true.
    // || (IS_NUMBER(value) && AS_NUMBER(value) == 0)
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

void VM::concatenate(){
    ObjString* b = AS_STRING(peek(0));
    ObjString* a = AS_STRING(peek(1));   // gc

    uint32_t length = a->array.length-1 + b->array.length-1;
    char* chars = (char*) gc.reallocate(nullptr, 0, sizeof(char) * (length+1));
    memcpy(chars, a->array.contents, a->array.length - 1);
    memcpy(chars + a->array.length - 1, b->array.contents, b->array.length - 1);
    chars[length] = '\0';

    ObjString* result = gc.takeString(chars, length);
    pop();
    pop();
    push(OBJ_VAL(result));
}

void VM::freeObjects(){
    Obj* object = gc.objects;
    while (object != nullptr) {
        Obj* next = object->next;
        gc.freeObject(object);
        object = next;
    }
    gc.objects = nullptr;
}

void VM::printDebugInfo() {
    Chunk* chunk = gc.frames[gc.frameCount - 1].closure->function->chunk;
    cout << "Current Chunk Constants:" << endl;
    chunk->printConstantsArray();
    cout << "Allocated Heap Objects:" << endl;
    printObjectsList(gc.objects);
    cout << "Current Stack:" << endl;
    printValueArray(gc.stack, gc.stackTop);
    cout << "Index in chunk: " << (ip - chunk->getCodePtr() - 1) << ". Length of chunk: " << chunk->getCodeSize()  << "." << endl;
    printStackTrace(&cout);
    cout << "Strings:" << endl;
    gc.strings->printContents();
}

void VM::loadInlineConstant(){
    unsigned char type = *ip;
    ip++;
    switch (type) {
        case 0: {
            double value;
            //assert(sizeof(value) == 8);
            memcpy(&value, ip, sizeof(value));
            ip += sizeof(value);
            currentChunk()->rawAddConstant(NUMBER_VAL(value), gc);
            break;
        }
        case (1 + OBJ_STRING): {
            uint32_t length;
            //assert(sizeof(length) == 4);
            memcpy(&length, ip, sizeof(length));
            ip += sizeof(length);
            ObjString* str = gc.copyString((char*) ip, length-1);
            currentChunk()->rawAddConstant(OBJ_VAL(str), gc);
            ip += length;
            break;
        }
        case (1 + OBJ_FUNCTION): {
            ObjFunction* func = gc.newFunction();
            func->arity = *ip;
            ip++;
            uint32_t length;
            //assert(sizeof(length) == 4);
            memcpy(&length, ip, sizeof(length));
            ip += sizeof(length);
            func->chunk->code->appendMemory(ip, length, gc);
            // TODO: pre-load the chunk constants. currently calling it a second time will double all the constants, etc
            ip += length;
            currentChunk()->rawAddConstant(OBJ_VAL(func), gc);
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
        *result = OBJ_VAL(gc.copyString(AS_CSTRING(array) + realIndex, 1));
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

        ObjString* newString = gc.copyString(AS_CSTRING(array) + realStartIndex, (int) (realEndIndex - realStartIndex));
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

bool VM::callValue(Value value, int argCount) {
    switch (value.type) {
        case VAL_OBJ:
            switch (AS_OBJ(value)->type) {
                case OBJ_CLOSURE:
                    return call(AS_CLOSURE(value), argCount);
                case OBJ_NATIVE: {
                    ObjNative* func = AS_NATIVE(value);
                    if (func->arity != argCount) {
                        FORMAT_RUNTIME_ERROR("Function call requires %d arguments, cannot pass %d.", func->arity, argCount);
                        return false;
                    }
                    Value result = func->function(this, gc.stackTop - argCount);
                    gc.stackTop -= argCount + 1;  // +1 for the object being called
                    push(result);
                    gc.frames[gc.frameCount - 1].ip = ip;
                    return true;
                }
                case OBJ_FUNCTION:
                    runtimeError("ICE. No direct function call. Must wrap with closure.");
                default:
                    break;
            }
            break;
    }
    runtimeError("Can only call functions and classes.");
    return false;
}

// remember to always use CACHE_FRAME() in the run loop.
bool VM::call(ObjClosure* closure, int argCount){
    ObjFunction* function = closure->function;
    if (gc.frameCount == FRAMES_MAX){
        runtimeError("Stack overflow.");
        return false;
    }

    if (argCount != function->arity) {
        FORMAT_RUNTIME_ERROR("Expected %d arguments but got %d.", function->arity, argCount);
        return false;
    }

    if (gc.frameCount > 0) gc.frames[gc.frameCount - 1].ip = ip;
    gc.frames[gc.frameCount].closure = closure;
    gc.frames[gc.frameCount].ip = function->chunk->getCodePtr();
    gc.frames[gc.frameCount].slots = gc.stackTop - argCount - 1;
    gc.frameCount++;
    return true;
}

ObjString *VM::produceString(const string& str) {
    return gc.copyString(str.c_str(), (int) str.length());
}

void VM::defineNative(const string& name, NativeFn function, int arity) {
    push(OBJ_VAL(produceString(name)));
    push(OBJ_VAL(gc.newNative(function, arity, AS_STRING(gc.stack[0]))));
    gc.natives->set(AS_STRING(peek(1)), peek(0));
    pop();
    pop();
}

#undef FORMAT_RUNTIME_ERROR