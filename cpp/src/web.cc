#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"
#include <emscripten.h>

#include <sstream>

// These are defined in worker.js
EM_JS(void, js_vm_print, (const char* ptr, unsigned long len, bool isErr), {});


void jsPrint(std::stringstream& out, bool isErr) {
    string s = out.str();  // TODO: copying here is stupid.
    js_vm_print(s.c_str(), s.length(), isErr);
    out.str("");
    out.clear();
}


class WebVm: public VM {
public:
    std::stringstream outBuffer;
    std::stringstream errBuffer;

    WebVm(): VM() {
        out = &outBuffer;
        err = &errBuffer;
        compiler.err = &errBuffer;
    }

    void afterPrint() override {
        jsPrint(outBuffer, false);
    }

    void runtimeError(const string& message) override {
        VM::runtimeError(message);
        jsPrint(errBuffer, true);
    }
};

WebVm vm;

extern "C" {
    void init_vm() {

    }

    void lox_run_src(char* src) {
        if (vm.loadFromSource(src)) {
            vm.run();
        } else {
            jsPrint(vm.errBuffer, true);
        }
    }
}
