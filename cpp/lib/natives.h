#include "../value.h"
#include "../common.h"
#include "../vm.h"
#include "../table.h"

namespace LoxNatives {
    Value klock(VM* vm, Value* args);
    Value time(VM* vm, Value* args);
    Value input(VM* vm, Value* args);
    Value eval(VM* vm, Value* args);
    static Table natives;
}