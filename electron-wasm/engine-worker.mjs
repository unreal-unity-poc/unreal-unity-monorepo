import { parentPort } from "node:worker_threads";
import { readFile } from "node:fs/promises";
import init, { WasmEngine } from "../wasm/pkg/rust_wasm_renderer.js";

const wasmBytes = await readFile(new URL("../wasm/pkg/rust_wasm_renderer_bg.wasm", import.meta.url));
const wasm = await init({ module_or_path: wasmBytes });
const engine = new WasmEngine();

let input = { rotateX: 0, rotateY: 0, zoom: 0, reset: 0 };
let mode = "transfer";
let shared = null;
let version = 0;

parentPort.on("message", (message) => {
  if (message.type === "configure") {
    mode = message.mode === "shared" && typeof SharedArrayBuffer !== "undefined"
      ? "shared"
      : "transfer";

    if (mode === "shared") {
      shared = new SharedArrayBuffer(frameByteLength());
      parentPort.postMessage({ type: "shared-ready", buffer: shared });
    }
    return;
  }

  if (message.type === "input") {
    input = message.input;
  }
});

function frameByteLength() {
  const patchCount = engine.patches_len();
  const floatCount = 9 + patchCount * 5;
  return 16 + floatCount * Float32Array.BYTES_PER_ELEMENT;
}

function writeFrame(buffer, isShared) {
  const header = new Int32Array(buffer, 0, 4);
  const floats = new Float32Array(buffer, 16);
  const patchCount = engine.patches_len();

  header[1] = patchCount;
  header[2] = 9 + patchCount * 5;

  const state = new Float32Array(wasm.memory.buffer, engine.render_state_ptr(), 9);
  floats.set(state, 0);

  const patches = new Float32Array(wasm.memory.buffer, engine.patches_ptr(), patchCount * 5);
  floats.set(patches, 9);

  if (isShared) {
    Atomics.store(header, 0, ++version);
  } else {
    header[0] = ++version;
  }
}

function tick() {
  engine.set_input(input.rotateX, input.rotateY, input.zoom, input.reset);
  engine.tick(1 / 60);

  if (mode === "shared" && shared) {
    writeFrame(shared, true);
  } else {
    const buffer = new ArrayBuffer(frameByteLength());
    writeFrame(buffer, false);
    parentPort.postMessage({ type: "frame", buffer }, [buffer]);
  }
}

setInterval(tick, 16);
