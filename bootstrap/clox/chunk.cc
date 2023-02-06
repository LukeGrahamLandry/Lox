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

void Chunk::writeConstant(Value value, int line){
    write(OP_CONSTANT, line);

    // TODO: another opcode for reading two bytes for the index for programs long enough to need more than 256
    if (constants->size() >= 256){
        cout << "Too Many Constants In Chunk" << endl;
        write(0, line);
        return;
    }

    constants->add(value);
    int location = constants->size() - 1;
    write(location, line);
}
