#include "chunk.h"

Chunk::Chunk(){
    code = new ArrayList<uint8_t>();
    constants = new ValueArray();
    lines = new ArrayList<int>();
}

Chunk::~Chunk(){
    delete code;
    delete constants;
    delete lines;
}

void Chunk::write(uint8_t byte, int line){
    code->add(byte);
    lines->add(line);
}

int Chunk::addConstant(Value value){
    constants->write(value);
    return constants->values->count - 1;
}
