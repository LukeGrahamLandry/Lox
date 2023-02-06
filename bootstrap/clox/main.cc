#include <fstream>
#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

char* readFile(const char* path);
void script(VM *vm, const char *path);
void repl(VM *vm);

int main(int argc, const char* argv[]) {
    VM* vm = new VM();

    vm->interpret("1 != 2 > 3 == 4 + 5 >> 6");
    return 0;

    if (argc == 1){
        repl(vm);
    } else if (argc == 2){
        script(vm, argv[1]);
    }

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

void script(VM *vm, const char *path) {
    char* src = readFile(path);
    InterpretResult result = vm->interpret(src);
    free(src);
    if (result == INTERPRET_COMPILE_ERROR) exit(65);
    if (result == INTERPRET_RUNTIME_ERROR) exit(70);
}

void repl(VM *vm) {
    char line[1024];

    for (;;){
        printf("> ");

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
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
