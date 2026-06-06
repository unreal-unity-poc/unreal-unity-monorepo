import { mkdirSync, existsSync } from "node:fs";
import { spawnSync } from "node:child_process";
import path from "node:path";

const root = path.resolve(new URL("..", import.meta.url).pathname);
const includeCandidates = [
  "/opt/homebrew/include/node",
  "/usr/local/include/node",
  path.resolve(path.dirname(process.execPath), "../include/node"),
];
const nodeInclude = includeCandidates.find((candidate) => existsSync(path.join(candidate, "node_api.h")));

if (!nodeInclude) {
  console.error("Could not find node_api.h. Checked:", includeCandidates.join(", "));
  process.exit(1);
}

if (process.platform !== "darwin") {
  console.error("The demo addon build script currently supports macOS only.");
  process.exit(1);
}

mkdirSync(path.join(root, "native"), { recursive: true });

const args = [
  "-std=c++17",
  "-O2",
  "-dynamiclib",
  "-undefined",
  "dynamic_lookup",
  "-fvisibility=hidden",
  `-I${nodeInclude}`,
  `-I${path.resolve(root, "../rust-engine/include")}`,
  path.join(root, "src/rust_engine_addon.cc"),
  "-o",
  path.join(root, "native/rust_engine_addon.node"),
];

const result = spawnSync("clang++", args, { stdio: "inherit" });
if (result.status !== 0) {
  process.exit(result.status ?? 1);
}
