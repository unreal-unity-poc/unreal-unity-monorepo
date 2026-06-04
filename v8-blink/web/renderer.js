// Intended V8 binding shape:
//
// const frame = rustEngine.frameArrayBuffer();
// const header = new Int32Array(frame, 0, 4);
// const floats = new Float32Array(frame, 16);
//
// Header:
// i32 version      offset 0
// i32 patchCount   offset 4
// i32 floatCount   offset 8
//
// Float payload:
// EarthRenderState, 9 f32 values
// SurfacePatch[], 5 f32 values per patch

export function readEarthFrame(frame) {
  const header = new Int32Array(frame, 0, 4);
  const patchCount = header[1];
  const floats = new Float32Array(frame, 16);

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

