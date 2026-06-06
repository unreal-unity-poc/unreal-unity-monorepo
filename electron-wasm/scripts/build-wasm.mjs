import { existsSync } from "node:fs";
import { spawnSync } from "node:child_process";
import path from "node:path";
import os from "node:os";

const env = { ...process.env };
const rustupStableBin = path.join(os.homedir(), ".rustup", "toolchains", "stable-aarch64-apple-darwin", "bin");

if (existsSync(path.join(rustupStableBin, "rustc"))) {
  env.PATH = `${rustupStableBin}:${env.PATH ?? ""}`;
}

const result = spawnSync("wasm-pack", ["build", "../wasm", "--target", "web"], {
  stdio: "inherit",
  env,
});

if (result.status !== 0) {
  process.exit(result.status ?? 1);
}
