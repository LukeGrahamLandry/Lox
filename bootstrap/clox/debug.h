#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

class Debugger {
    public:
    Debugger();

    Debugger(Chunk* chunk);

        Chunk* chunk;

        void debug(const string& name);
        int simpleInstruction(const string& name, int offset);
        int constantInstruction(const string& name, int offset);
        int debugInstruction(int offset);
        void setChunk(Chunk* chunk);
    };

#endif