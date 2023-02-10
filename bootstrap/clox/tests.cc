#include <sstream>
#include "chunk.h"
#include "vm.h"
#include "common.h"

void runTest(bool (*func)(), const char* name);
bool testChunkExports();
void runOutputTest(const char* name, const char* code, const char* expectedOutput);

int passed = 0;
int total = 0;
string simpleCode = string("var x = 10; x = 50; { print x + 2 / 2; } var hello = \"hello\"; print \"hi\" + \" \" + hello + \".\";");
const char* binaryPath = "code.blox";

int main() {
    runTest(&testChunkExports, "Export chunks as binary");
    runOutputTest(
            "Index into strings",
            "print \"hello\"[0]; print \"hello\"[-1]; print \"hello\"[:2]; print \"hello\"[2:]; print \"hello\"[:]; print \"hello\"[1:4]; print \"hello\"[1:-1];",
            "h\no\nhe\nllo\nhello\nell\nell\n"
    );

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

void runTest(bool (*func)(), const char* name) {
    string n = name;
    startTest(n);
    bool result = func();
    endTest(n, result);
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

bool testChunkExports() {
    ostringstream buffer1;
    ostringstream buffer2;
    const char* src = simpleCode.c_str();

    VM vm;
    vm.setOutput(&buffer1);
    if (!vm.loadFromSource(const_cast<char *>(src))) return false;
    vm.run();

    vm.getChunk()->exportAsBinary(binaryPath);

    VM vm2;
    vm.setOutput(&buffer2);
    Chunk* chunk = Chunk::importFromBinary(binaryPath);
    vm.setChunk(chunk);
    vm.run();

    bool same = buffer1.str() == buffer2.str();
    if (!same){
        cout << "Output from source code vm: " << endl << buffer1.str() << endl;
        cout << "Output from imported bytecode vm: " << endl << buffer2.str() << endl;
        cout << "Expected output to match." << endl;
    }

    return same;
}
