#include "chunk.h"

Chunk::Chunk(){
    code = new ArrayList<uint8_t>();
    constants = new ValueArray();
    lines = new ArrayList<int>();  // run-length encoded: count, line, count, line
}

Chunk::~Chunk(){
    delete code;
    delete constants;
    delete lines;
}

Chunk::Chunk(const Chunk& other){
    code = new ArrayList<uint8_t>(*other.code);
    constants = new ValueArray();
    delete constants->values;
    constants->values = new ArrayList<Value>(*other.constants->values);
    lines = new ArrayList<int>(*other.lines);
}


void Chunk::write(uint8_t byte, int line){
    code->add(byte);

    if (!lines->isEmpty()){
        int prevLine = lines->get(lines->count - 1);
        if (line == prevLine){
            int prevCount = lines->get(lines->count - 2);
            lines->set(lines->count - 2, prevCount + 1);
            return;
        }
    }

    lines->add(1);
    lines->add(line);
}

// This has O(n) but since it will only be called while debugging or when an exception is thrown, it doesn't matter.
int Chunk::getLineNumber(int opcodeOffset){
    int currentTokenIndex = 0;
    for (int i=0;i<lines->count-1;i+=2){
        int count = lines->get(i);
        int lineNumber = lines->get(i + 1);
        currentTokenIndex += count;
        if (opcodeOffset <= currentTokenIndex) return lineNumber;
    }

    return -1;
}

// Adds a constant to the array (without a push op code).
// Ownership of the value's heap memory (if an object) is passed to the chunk.
const_index_t Chunk::addConstant(Value value){
    // TODO: another opcode for reading two bytes for the index for programs long enough to need more than 256
    if (constants->size() >= 256){
        cerr << "Too Many Constants In Chunk" << endl;
        return 0;
    }

    // Loop through all existing constants to deduplicate.
    for (int i=0;i<constants->size();i++){
        Value constant = constants->get(i);
        if (valuesEqual(value, constant)){
            if (IS_OBJ(value)){ // The Obj struct is allocated on the heap.
                bool sameAddress = AS_OBJ(value) == AS_OBJ(constant);
                if (!sameAddress){
                    switch (AS_OBJ(value)->type) {
                        case OBJ_STRING:
                            FREE(ObjString, AS_STRING(value));
                            break;
                    }
                }
            }
            return i;
        }
    }

    constants->add(value);
    return constants->size() - 1;
}

int Chunk::getCodeSize() {
    return code->count;
}

unsigned char *Chunk::getCodePtr() {
    return code->data;
}

void Chunk::printConstantsArray() {
    printValueArray(constants->values->data, constants->values->data + constants->size());
}

Value Chunk::getConstant(int index) {
    return constants->get(index);
}
