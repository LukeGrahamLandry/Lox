#ifndef clox_compiler_h
#define clox_compiler_h

class Compiler {
    public:
        Compiler();
        ~Compiler();

        void compile(char *src);
};

#endif
