#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char* argv[]) {
    VM* vm = new VM();

    Chunk* test = new Chunk();

    test->writeConstant(1, 123);
    test->writeConstant(5, 123);
    test->write(OP_ADD, 123);
    test->writeConstant(2, 123);
    test->write(OP_DIVIDE, 123);
    test->write(OP_NEGATE, 123);
    test->write(OP_RETURN, 123);

    vm->interpret(test);

    delete test;
    delete vm;
    return 0;
}
