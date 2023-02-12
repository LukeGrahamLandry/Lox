#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

char* readFile(const char* path);
void script(VM *vm, const char *path);
void bytecode(VM *vm, const char *path);
void repl(VM *vm);

int main(int argc, const char* argv[]) {
    VM* vm = new VM();

    if (argc == 1){
        repl(vm);
    } else if (argc == 2){
        script(vm, argv[1]);
    } else if (argc == 3 && strcmp(argv[1], "-b") == 0){
        bytecode(vm, argv[2]);
    } else if (argc == 3 && strcmp(argv[1], "-s") == 0){
        Debugger::silent = true;
        script(vm, argv[2]);
    }

    delete vm;
    return 0;
}

void bytecode(VM *vm, const char *path) {
    Chunk* chunk = Chunk::importFromBinary(path);
    vm->setChunk(chunk);
    vm->run();
}

void script(VM *vm, const char *path) {
    char* src = readFile(path);

    InterpretResult result = INTERPRET_COMPILE_ERROR;
    if (vm->loadFromSource(src)){
        for (;;){
            result = vm->run();
            if (result == INTERPRET_DEBUG_BREAK_POINT) {
                long offset = vm->ip - vm->getChunk()->getCodePtr();
                Chunk* original = new Chunk(*vm->getChunk());  // copy
                repl(vm);
                vm->setChunk(original);  // ownership of <original>'s memory is given back to the vm
                vm->ip = original->getCodePtr() + offset;
            }
            else break;
        }
    }

    free(src);
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
