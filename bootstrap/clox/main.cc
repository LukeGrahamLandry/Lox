#include "common.h"
#include "chunk.h"
#include "debug.h"

int main(int argc, const char* argv[]) {
    Chunk* test = new Chunk();

    int location = test->addConstant(1.4);
    test->write(OP_CONSTANT, 123);
    test->write(location, 123);
    test->write(OP_RETURN, 123);
    
    Debugger debug = Debugger(test);
    debug.debug("test");

    delete test;
    return 0;
}
