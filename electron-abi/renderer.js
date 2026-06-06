const path = require("node:path");
const addon = require("./native/rust_engine_addon.node");

const canvas = document.querySelector("#scene");
const context = canvas.getContext("2d");
const keys = new Set();
let queuedWheelZoom = 0;
let lastFrameAt = performance.now();

const libraryPath = path.join(__dirname, "native", "macos", "librust_engine.dylib");
addon.initialize(libraryPath);

resizeCanvas();
window.addEventListener("resize", resizeCanvas);
window.addEventListener("beforeunload", () => addon.shutdown());
window.addEventListener("keydown", (event) => {
  if (isControlKey(event.key)) event.preventDefault();
  keys.add(event.key.toLowerCase());
});
window.addEventListener("keyup", (event) => {
  if (isControlKey(event.key)) event.preventDefault();
  keys.delete(event.key.toLowerCase());
});
window.addEventListener(
  "wheel",
  (event) => {
    event.preventDefault();
    const direction = Math.sign(-event.deltaY);
    if (direction !== 0) queuedWheelZoom += direction * 6;
  },
  { passive: false },
);

function resizeCanvas() {
  const scale = window.devicePixelRatio || 1;
  canvas.width = Math.floor(window.innerWidth * scale);
  canvas.height = Math.floor(window.innerHeight * scale);
}

function isControlKey(key) {
  return ["ArrowUp", "ArrowDown", "ArrowLeft", "ArrowRight", "+", "=", "-", "PageUp", "PageDown", "r", "R"].includes(key);
}

function readInput() {
  let rotateX = 0;
  let rotateY = 0;
  let zoom = 0;

  if (keys.has("arrowup")) rotateX += 1;
  if (keys.has("arrowdown")) rotateX -= 1;
  if (keys.has("arrowleft")) rotateY += 1;
  if (keys.has("arrowright")) rotateY -= 1;
  if (keys.has("=") || keys.has("+") || keys.has("pageup")) zoom += 1;
  if (keys.has("-") || keys.has("pagedown")) zoom -= 1;

  const queuedZoomForFrame = Math.max(-1, Math.min(1, queuedWheelZoom));
  zoom += queuedZoomForFrame;
  queuedWheelZoom -= queuedZoomForFrame;
  if (Math.abs(queuedWheelZoom) < 0.01) queuedWheelZoom = 0;

  return { rotateX, rotateY, zoom, reset: keys.has("r") ? 1 : 0 };
}

function decodeFrame(buffer) {
  const header = new Int32Array(buffer, 0, 4);
  const floats = new Float32Array(buffer, 16);
  const patchCount = header[1];
  const state = {
    radius: floats[0],
    atmosphereRadius: floats[1],
    rotationX: floats[2],
    rotationY: floats[3],
    cloudRotationY: floats[4],
    cameraDistance: floats[5],
    lightX: floats[6],
    lightY: floats[7],
    lightZ: floats[8],
  };
  const patches = [];
  let offset = 9;
  for (let index = 0; index < patchCount; index += 1) {
    patches.push({
      latDegrees: floats[offset + 0],
      lonDegrees: floats[offset + 1],
      radiusDegrees: floats[offset + 2],
      stretchX: floats[offset + 3],
      stretchY: floats[offset + 4],
    });
    offset += 5;
  }
  return { state, patches };
}

function sphericalNormal(latDegrees, lonDegrees) {
  const lat = latDegrees * Math.PI / 180;
  const lon = lonDegrees * Math.PI / 180;
  const cosLat = Math.cos(lat);
  return { x: cosLat * Math.sin(lon), y: Math.sin(lat), z: cosLat * Math.cos(lon) };
}

function rotate(vector, state) {
  const cosX = Math.cos(state.rotationX);
  const sinX = Math.sin(state.rotationX);
  const y = vector.y * cosX - vector.z * sinX;
  const z = vector.y * sinX + vector.z * cosX;
  vector = { x: vector.x, y, z };

  const cosY = Math.cos(state.rotationY);
  const sinY = Math.sin(state.rotationY);
  return {
    x: vector.x * cosY + vector.z * sinY,
    y: vector.y,
    z: -vector.x * sinY + vector.z * cosY,
  };
}

function drawFrame(frame) {
  const { state, patches } = frame;
  context.fillStyle = "#080b12";
  context.fillRect(0, 0, canvas.width, canvas.height);

  const center = { x: canvas.width * 0.5, y: canvas.height * 0.5 };
  const globeRadius = Math.min(canvas.width, canvas.height) * 0.34 * (4.2 / state.cameraDistance);
  const ocean = context.createRadialGradient(
    center.x - globeRadius * 0.34,
    center.y - globeRadius * 0.34,
    globeRadius * 0.05,
    center.x,
    center.y,
    globeRadius * 1.1,
  );
  ocean.addColorStop(0, "#3f91e7");
  ocean.addColorStop(0.55, "#155aad");
  ocean.addColorStop(1, "#04133e");

  context.save();
  context.beginPath();
  context.arc(center.x, center.y, globeRadius, 0, Math.PI * 2);
  context.clip();
  context.fillStyle = ocean;
  context.fillRect(center.x - globeRadius, center.y - globeRadius, globeRadius * 2, globeRadius * 2);

  for (const patch of patches) {
    const normal = rotate(sphericalNormal(patch.latDegrees, patch.lonDegrees), state);
    if (normal.z < -0.08) continue;

    const patchRadius = globeRadius * patch.radiusDegrees / 90;
    const x = center.x + normal.x * globeRadius;
    const y = center.y - normal.y * globeRadius;

    context.save();
    context.translate(x, y);
    context.rotate(state.rotationY * 0.35 + patch.lonDegrees * Math.PI / 180);
    context.scale(patch.stretchX, patch.stretchY);
    context.fillStyle = "#288345";
    context.beginPath();
    context.ellipse(0, 0, patchRadius, patchRadius, 0, 0, Math.PI * 2);
    context.fill();
    context.restore();
  }

  context.restore();
  context.strokeStyle = "rgba(136, 203, 255, 0.58)";
  context.lineWidth = Math.max(2, globeRadius * 0.014);
  context.beginPath();
  context.arc(center.x, center.y, globeRadius * state.atmosphereRadius, 0, Math.PI * 2);
  context.stroke();
}

function frame(now) {
  const input = readInput();
  const dt = Math.max(1 / 120, Math.min(1 / 30, (now - lastFrameAt) / 1000));
  lastFrameAt = now;

  const buffer = addon.tickFrame(input.rotateX, input.rotateY, input.zoom, input.reset, dt);
  drawFrame(decodeFrame(buffer));
  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);
