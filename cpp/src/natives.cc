#include "natives.h"
#include <chrono>
#include <iostream>
#include <string>

Value LoxNatives::klock(VM* vm, Value* args) {
    return NUMBER_VAL((double)clock() / CLOCKS_PER_SEC);
}

Value LoxNatives::time(VM* vm, Value* args) {
    long time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
    return NUMBER_VAL((double) time / 1000);
}

// TODO: halt and wait for user
Value LoxNatives::input(VM* vm, Value* args) {
    string line;
    getline(std::cin, line);
    ObjString* str = vm->produceString(line);
    return OBJ_VAL(str);
}

// TODO: remove. I feel like nobody sane wants this.
Value LoxNatives::eval(VM* vm, Value* args) {
    char* code = AS_CSTRING(args[0]);
    return vm->produceFunction(code);
}
