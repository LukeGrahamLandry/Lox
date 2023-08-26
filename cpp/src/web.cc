#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

extern "C" {

void lox_run_src(char* src) {
    VM vm;
    vm.loadFromSource(src);
    vm.run();
}

}