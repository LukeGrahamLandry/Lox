#include <sstream>
#include "../chunk.h"
#include "../vm.h"
#include "../common.h"

void runOutputTest(const char* name, const char* code, const char* expectedOutput);
string read(const char* path);

// TODO: copy paste
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

int passed = 0;
int total = 0;

int main() {
    runOutputTest(
            "Index into strings",
            readFile("../tests/case/strings.lox"),
            "h\no\nhe\nllo\nhello\nell\nell\n"
    );

    runOutputTest("functions", readFile("../tests/case/functions.lox"), "done\n");

    cout << "Passed " << passed << " of " << total << " tests." << endl;
    return 0;
}

void startTest(string& name){
    total++;
    cout << "START TEST " << total << ": " << name << "." << endl;
}

void endTest(string& name, bool result){
    if (result) cout << "PASS";
    else cout << "FAIL";
    cout << " TEST " << total << ": " << name << "." << endl;
    passed += result;
}

void runOutputTest(const char* name, const char* code, const char* expectedOutput){
    string n = name;
    string c = code;
    string o = expectedOutput;
    startTest(n);
    bool result = false;

    ostringstream buffer;
    VM vm;
    if (vm.loadFromSource(const_cast<char *>(c.c_str()))) {
        vm.setOutput(&buffer);
        vm.run();
        result = buffer.str() == o;
        if (!result){
            cout << "Expected output: " << endl << o << endl;
            cout << "Actual output: " << endl << buffer.str() << endl;
        }
    }

    endTest(n, result);
}

