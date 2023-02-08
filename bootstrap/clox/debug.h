#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

class Debugger {
public:
    Debugger();
    Debugger(Chunk* chunk);
    void setChunk(Chunk* chunk);
    void debug(const string& name);
    int debugInstruction(int offset);


private:
    Chunk* chunk;
    int lastLine;

    int simpleInstruction(const string& name, int offset);
    int constantInstruction(const string& name, int offset);
};

#endif