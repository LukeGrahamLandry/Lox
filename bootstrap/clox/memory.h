#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

void* reallocate(void* pointer, size_t oldSize, size_t newSize);

#endif
