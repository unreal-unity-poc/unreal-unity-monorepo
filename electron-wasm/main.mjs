import { app, BrowserWindow, ipcMain } from "electron";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { Worker } from "node:worker_threads";

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const workersByWebContentsId = new Map();

function createWindow() {
  const window = new BrowserWindow({
    title: "Rust Electron WASM Renderer",
    width: 1228,
    height: 768,
    backgroundColor: "#080b12",
    webPreferences: {
      contextIsolation: false,
      nodeIntegration: true,
      sandbox: false,
    },
  });

  window.setTitle("Rust Electron WASM Renderer");
  window.loadFile(path.join(__dirname, "index.html"));

  const worker = new Worker(path.join(__dirname, "engine-worker.mjs"), { type: "module" });
  const webContentsId = window.webContents.id;
  workersByWebContentsId.set(webContentsId, worker);

  worker.on("message", (message) => {
    if (!window.isDestroyed()) {
      window.webContents.send("rust-frame", message);
    }
  });
  worker.on("error", (error) => {
    if (!window.isDestroyed()) {
      window.webContents.send("rust-worker-error", error.message);
    }
  });
  worker.on("exit", (code) => {
    workersByWebContentsId.delete(webContentsId);
    if (code !== 0 && !window.isDestroyed()) {
      window.webContents.send("rust-worker-error", `worker exited with code ${code}`);
    }
  });

  window.webContents.once("did-finish-load", () => {
    worker.postMessage({ type: "configure", mode: "transfer" });
  });

  window.on("closed", () => {
    worker.terminate();
    workersByWebContentsId.delete(webContentsId);
  });
}

app.whenReady().then(createWindow);

ipcMain.on("rust-input", (event, input) => {
  const worker = workersByWebContentsId.get(event.sender.id);
  if (worker) {
    worker.postMessage({ type: "input", input });
  }
});

app.on("window-all-closed", () => {
  app.quit();
});
