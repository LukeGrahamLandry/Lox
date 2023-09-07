#ifndef clox_common_h
#define clox_common_h

#include <cstdint>
#include <cstddef>
#include <iostream>

using namespace std;

// TODO: its annoying that running the tests needs you to turn these off.
#define COMPILER_DEBUG_PRINT_CODE
#define VM_DEBUG_TRACE_EXECUTION
#define VM_ALLOW_DEBUG_BREAK_POINT
#define VM_SAFE_MODE
#define VM_PROFILING

#define byte uint8_t
#define cast(targetType, v) (reinterpret_cast<targetType>(v))

#define FRAMES_MAX 64
#define STACK_MAX (FRAMES_MAX * 256)

#endif