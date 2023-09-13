#include "chunk.h"
#include "vm.h"
#include "object.h"

Chunk::Chunk(){
    code = new ArrayList<byte>();
    constants = new ArrayList<Value>();
    lines = new ArrayList<int>();  // run-length encoded: count, line, count, line
}

Chunk::~Chunk(){
    delete code;
    delete constants;
    delete lines;
}

void Chunk::release(Memory& gc){
    code->release(gc);
    constants->release(gc);
    lines->release(gc);
}

Chunk::Chunk(const Chunk& other){
    code = new ArrayList<byte>(*other.code);
    delete constants;
    constants = new ArrayList<Value>(*other.constants);
    lines = new ArrayList<int>(*other.lines);
}


void Chunk::write(byte b, int line, Memory& gc){
    code->push(b, gc);

    if (!lines->isEmpty()){
        int prevLine = (*lines)[lines->count - 1];
        if (line == prevLine){
            int prevCount = (*lines)[lines->count - 2];
            lines->set(lines->count - 2, prevCount + 1);
            return;
        }
    }

    lines->push(1, gc);
    lines->push(line, gc);
}

// This has O(n) but since it will only be called while debugging or when an exception is thrown, it doesn't matter.
int Chunk::getLineNumber(int opcodeOffset){
    if (lines->isEmpty()) return -1;

    int currentTokenIndex = 0;
    for (int i=0;i<lines->count-1;i+=2){
        int count = (*lines)[i];
        int lineNumber = (*lines)[i+1];
        currentTokenIndex += count;
        if (opcodeOffset <= currentTokenIndex) return lineNumber;
    }

    return -1;
}

// Adds a constant to the array (without a push op code).
// Ownership of the value's heap memory (if an object) is passed to the chunk.
const_index_t Chunk::addConstant(Value value, Memory& gc){
    // TODO: another opcode for reading two bytes for the index for programs long enough to need more than 256
    if (constants->size() >= 256){
        cerr << "Too many constants in chunk." << endl;
        return -1;
    }

    if (IS_NIL(value) || IS_BOOL(value)){
        cerr << "Cannot add singleton value to chunk constants." << endl;
        return -1;
    }

    // Loop through all existing constants to deduplicate.
    for (int i=0;i<constants->size();i++){
        Value constant = (*constants)[i];
        if (valuesEqual(value, constant)){
            if (IS_OBJ(value)){ // The Obj struct is allocated on the heap.
                bool sameAddress = AS_OBJ(value) == AS_OBJ(constant);
                if (!sameAddress){  // if the new Value is a different address but the same data, we delete it and use the old one instead
                    switch (AS_OBJ(value)->type) {
                        case OBJ_STRING:
                            gc.FREE(ObjString, AS_STRING(value));
                            break;
                    }
                }
            }
            return i;
        }
    }

    rawAddConstant(value, gc);
    return constants->size() - 1;
}

void Chunk::rawAddConstant(Value value, Memory& gc){
    gc.push(value);
    constants->push(value, gc);
    gc.pop();
}

int Chunk::getCodeSize() {
    return (int) code->count;
}

int Chunk::getConstantsSize(){
    return constants->size();
}

unsigned char *Chunk::getCodePtr() {
    return code->data;
}

unsigned char Chunk::getInstruction(int offset) {
    uint32_t index = offset < 0 ? code->count + offset: offset;
    return (*code)[index];
}

int Chunk::popInstruction() {
    return code->pop();
}

void Chunk::printConstantsArray() {
    debugPrintValueArray(constants->data, constants->data + constants->size());
}

Value Chunk::getConstant(int index) {
    return (*constants)[index];
}


void Chunk::setCodeAt(int index, byte value){
    code->set(index, value);
}

// When done compiling the function, the chunk is effectively immutable, so we can remove the extra list space.
void Chunk::setDone(Memory& gc){
    constants->shrink(gc);
    code->shrink(gc);
    lines->shrink(gc);
}

#define OP(name) [name] = #name,

// This is used for printing profiling info.
string Chunk::opcodeNames[256] = {
        OP(OP_GET_CONSTANT)
        OP(OP_NIL)
        OP(OP_TRUE)
        OP(OP_FALSE)
        OP(OP_EQUAL)
        OP(OP_GREATER)
        OP(OP_LESS)
        OP(OP_ADD)
        OP(OP_SUBTRACT)
        OP(OP_MULTIPLY)
        OP(OP_DIVIDE)
        OP(OP_NEGATE)
        OP(OP_NOT)
        OP(OP_RETURN)
        OP(OP_EXPONENT)
        OP(OP_PRINT)
        OP(OP_POP)
        OP(OP_POP_MANY)
        OP(OP_DEFINE_GLOBAL)
        OP(OP_DEBUG_BREAK_POINT)
        OP(OP_EXIT_VM)
        OP(OP_ACCESS_INDEX)
        OP(OP_SLICE_INDEX)
        OP(OP_GET_LENGTH)
        OP(OP_GET_LOCAL)
        OP(OP_SET_LOCAL)
        OP(OP_LOAD_INLINE_CONSTANT)
        OP(OP_METHOD)
        OP(OP_GET_PROPERTY)
        OP(OP_SET_PROPERTY)
        OP(OP_CLOSE_UPVALUE)
        OP(OP_CLASS)
        OP(OP_CLOSURE)
        OP(OP_GET_UPVALUE)
        OP(OP_SET_UPVALUE)
        OP(OP_CALL)
        OP(OP_JUMP_IF_FALSE)
        OP(OP_LOOP)
        OP(OP_JUMP)
        OP(OP_INVOKE)
};

#undef OP