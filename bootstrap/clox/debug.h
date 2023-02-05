#ifndef clox_debug_h
#define clox_debug_h

#include "chunk.h"

class Debugger {
    public:
        Debugger(Chunk* chunk);

        Chunk* chunk;

        void debug(string name);
        int simpleInstruction(const string name, int offset);
        int constantInstruction(const string name, int offset);
        int debugInstruction(int offset);
    };

#endif