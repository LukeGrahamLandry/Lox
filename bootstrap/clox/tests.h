#include "chunk.h"
#include "vm.h"
#include "common.h"

void runAllTests();
void runTest(bool (*func)(), const char* name);
bool testChunkExports();