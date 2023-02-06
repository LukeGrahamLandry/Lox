#include "compiler.h"
#include "scanner.h"
#include "common.h"

Compiler::Compiler(){

};

Compiler::~Compiler(){

};

void Compiler::compile(char* src){
    Scanner scanner = Scanner(src);

    int line = -1;
    for (;;) {
        Token token = scanner.scanToken();
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }
        printf("%2d '%.*s'\n", token.type, token.length, token.start);

        if (token.type == TOKEN_EOF) break;
    }
}
