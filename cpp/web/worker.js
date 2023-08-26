function runLox(src) {
    console.log("RUN:", src);
    let srcPtr = _malloc(src.length);
    stringToUTF8(src, srcPtr, src.length);  // TODO: not enough space for non-ascii
    _lox_run_src(srcPtr);
    _free(srcPtr);
    self.postMessage({ action: "finished" });
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