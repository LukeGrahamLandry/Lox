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
    static bool silent;


private:
    Chunk* chunk;
    int lastLine;
    int inlineConstantCount;

    int simpleInstruction(const string& name, int offset);
    int constantInstruction(const string& name, int offset);

    int jumpInstruction(const char *name, int sign, Chunk *chunk, int offset);
};

#endif