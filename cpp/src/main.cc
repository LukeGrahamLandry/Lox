#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

char* readFile(const char* path);
void script(VM *vm, const char *path);
void bytecode(VM *vm, const char *path);
void repl(VM *vm);

int main(int argc, const char* argv[]) {
    VM vm;

    if (argc == 1){
        repl(&vm);
    } else if (argc == 2){
        script(&vm, argv[1]);
    } else if (argc == 3 && strcmp(argv[1], "-s") == 0){
        Debugger::silent = true;
        script(&vm, argv[2]);
    }
//
//    char* src = "fun fib(n) {\n"
//                "  if (n < 2) return n;\n"
//                "  return fib(n - 2) + fib(n - 1);\n"
//                "}\n"
//                "\n"
//                "print fib(4);";
//    vm.loadFromSource(src);
//    vm.run();

    return 0;
}

void script(VM *vm, const char *path) {
    char* src = readFile(path);

    InterpretResult result = INTERPRET_COMPILE_ERROR;
    if (vm->loadFromSource(src)){
        for (;;){
            result = vm->run();
            break;
        }
    }

    free(src);
    if (result == INTERPRET_OK) exit(vm->exitCode);
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

void repl(VM *vm) {
    char line[1024];

    for (int i=0;;i++){
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        if (vm->loadFromSource(line)){
            InterpretResult result;
            for (;;){
                result = vm->run();
                if (result == INTERPRET_DEBUG_BREAK_POINT) {
                    vm->printDebugInfo();
                }
                else break;
            }

            if (result == INTERPRET_EXIT) break;
        }
    }
}

char* readFile(const char* path) {
    FILE* file = fopen(path, "rb");

    if (file == NULL) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        exit(74);
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file);
    rewind(file);

    char* buffer = (char*)malloc(fileSize + 1);
    size_t bytesRead = fread(buffer, sizeof(char), fileSize, file);
    buffer[bytesRead] = '\0';

    if (buffer == NULL) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        exit(74);
    }

    fclose(file);

    if (bytesRead < fileSize) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        exit(74);
    }

    return buffer;
}
