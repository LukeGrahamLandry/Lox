function runLox(src) {
    console.log("RUN:", src);
    let srcPtr = _malloc(src.length + 1);  // +1 for null terminator
    stringToUTF8(src, srcPtr, src.length + 1);  // TODO: not enough space for non-ascii
    _lox_run_src(srcPtr);
    _free(srcPtr);
    self.postMessage({ action: "finished" });
}

function js_vm_print(ptr, len, isErr) {
    let str = UTF8ToString(ptr, len); // TODO: this still checks for a null terminator in the middle which is silly.
    self.postMessage( { action: "print", msg: str, isErr: isErr});
}

self.onmessage = (msg) => {
    if (msg.data.action === "run") {
        runLox(msg.data.srcCode);
    } else if (msg.data.action === "isready") {
        console.log("VM worker ready.");
    } else {
        console.log("Invalid message", msg);
    }
};

// Called by emscripten
Module.onRuntimeInitialized = function (){
    _init_vm();
}
