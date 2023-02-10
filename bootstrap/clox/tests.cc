#include "tests.h"
#include <sstream>

int passed = 0;
int total = 0;
string simpleCode = string("var x = 10; x = 50; { print x + 2 / 2; } var hello = \"hello\"; print \"hi\" + \" \" + hello + \".\";");
const char* binaryPath = "code.blox";

int main() {
    runTest(&testChunkExports, "Export chunks as binary");

    cout << "Passed " << passed << " of " << total << " tests." << endl;
    return 0;
}

void runTest(bool (*func)(), const char* name) {
    total++;
    cout << "START Test " << total << ": " << name << "." << endl;
    bool result = func();

    if (result) cout << "PASS";
    else cout << "FAIL";
    cout << " Test " << total << ": " << name << "." << endl;
    passed += result;
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
