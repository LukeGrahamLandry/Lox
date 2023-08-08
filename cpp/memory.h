#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

#define ALLOCATE(type, length) (type*) reallocate(nullptr, 0, sizeof(type) * length)

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

#define FREE_ARRAY(type, pointer, count) reallocate(pointer, sizeof(type) * count, 0)

void* reallocate(void* pointer, size_t oldSize, size_t newSize);
void* reallocate(void* pointer, size_t oldSize, size_t newSize, bool allowGC);
void collectGarbage();
void markRoots();

typedef struct Value Value;
typedef struct Obj Obj;

void markValue(Value value);
void markObject(Obj* object);

#endif
