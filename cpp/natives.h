#include "value.h"
#include "common.h"
#include "vm.h"
#include "table.h"

// TODO: add the extensions to run loxlox, seems like a good test, https://benhoyt.com/writings/loxlox/
namespace LoxNatives {
    Value klock(VM* vm, Value* args);
    Value time(VM* vm, Value* args);
    Value input(VM* vm, Value* args);
    Value eval(VM* vm, Value* args);
}