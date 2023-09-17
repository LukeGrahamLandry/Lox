if (typeof(Worker) === "undefined") {
    alert("Your browser does not support web workers.");
}

let loxWorker = null;

function handleRunClick() {
    document.getElementById("output").value = "";
    const src = document.getElementById("code").value;
    setWaitingForVm(true);
    loxWorker.postMessage({
        action: "run",
        srcCode: src
    })
}

function handleLoxMessage(msg){
    switch (msg.data.action) {
        case "finished": {
            setWaitingForVm(false);
            break;
        }
        case "print": {
            let out = document.getElementById("output");
            if (msg.data.isErr) {
                out.value += "Error:\n";
            }
            out.value += msg.data.msg;
            out.scrollTo(0, out.scrollHeight);
            break;
        }
    }
}

function setWaitingForVm(isActive) {
    document.getElementById("run").disabled = isActive;
    document.getElementById("halt").disabled = !isActive;
}

// This would be slow to call many times because it recompiles the wasm.
// Only use if the user triggered it because of an infinite loop or something.
function forceResetWorker() {
    if (loxWorker != null) {
        loxWorker.terminate();
    }
    loxWorker = new Worker("workerbundle.js");
    loxWorker.onmessage = handleLoxMessage;
    loxWorker.postMessage({ action: "isready" });
    setWaitingForVm(false);
}

forceResetWorker();
