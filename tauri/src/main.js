const canvas = document.querySelector("#scene");
const context = canvas.getContext("2d");
const keys = new Set();
let lastTime = performance.now();

const invoke = window.__TAURI__.core.invoke;

window.addEventListener("keydown", (event) => keys.add(event.key.toLowerCase()));
window.addEventListener("keyup", (event) => keys.delete(event.key.toLowerCase()));

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

  return {
    rotateX,
    rotateY,
    zoom,
    reset: keys.has("r") ? 1 : 0,
  };
}

function sphericalNormal(latDegrees, lonDegrees) {
  const lat = latDegrees * Math.PI / 180;
  const lon = lonDegrees * Math.PI / 180;
  const cosLat = Math.cos(lat);
  return {
    x: cosLat * Math.sin(lon),
    y: Math.sin(lat),
    z: cosLat * Math.cos(lon),
  };
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

function draw(payload) {
  const { state, patches } = payload;
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
    context.scale(patch.stretchX, patch.stretchY);
    context.fillStyle = "#288345";
    context.beginPath();
    context.ellipse(0, 0, patchRadius, patchRadius, 0, 0, Math.PI * 2);
    context.fill();
    context.restore();
  }

  context.restore();
}

async function frame(now) {
  const dt = Math.min((now - lastTime) / 1000, 0.1);
  lastTime = now;

  const payload = await invoke("tick", {
    input: readInput(),
    dtSeconds: dt,
  });
  draw(payload);

  requestAnimationFrame(frame);
}

requestAnimationFrame(frame);

