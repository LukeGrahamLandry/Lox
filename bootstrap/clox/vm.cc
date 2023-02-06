#include "vm.h"
#include "compiler.h"
#include <cmath>
VM::VM() {
    resetStack();
    compiler = new Compiler();
    #ifdef DEBUG_TRACE_EXECUTION
    debug = new Debugger();
    #endif
}

VM::~VM() {
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

InterpretResult VM::run() {
    #define READ_BYTE() (*(ip++))
    #define READ_CONSTANT() chunk->constants->get(READ_BYTE())
    // fucky thing to make it one statement and allow a ; on the end
    #define BINARY_OP(op)      \
    do {                       \
       Value right = pop();    \
       Value left = pop();     \
       push(left op right);    \
    } while (false)

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
            case OP_CONSTANT:
                push(READ_CONSTANT());
                break;
            case OP_ADD:
                BINARY_OP(+);
                break;
            case OP_SUBTRACT:
                BINARY_OP(-);
                break;
            case OP_MULTIPLY:
                BINARY_OP(*);
                break;
            case OP_DIVIDE:
                BINARY_OP(/);
                break;
            case OP_EXPONENT: {
                Value right = pop();
                Value left = pop();
                push(pow(left, right));
                break;
            }
            case OP_NEGATE:
                push(-pop());
                break;
            case OP_RETURN:
                printValue(pop());
                cout << endl;
                return INTERPRET_OK;
        }
    }

    #undef READ_BYTE
    #undef READ_CONSTANT
}
