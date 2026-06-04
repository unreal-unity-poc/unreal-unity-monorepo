use rust_engine::{ControlInput, EarthRenderState, Engine, SurfacePatch};
use serde::{Deserialize, Serialize};
use std::sync::Mutex;
use tauri::State;

#[derive(Debug, Deserialize)]
#[serde(rename_all = "camelCase")]
struct InputDto {
    rotate_x: f32,
    rotate_y: f32,
    zoom: f32,
    reset: u32,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct EarthStateDto {
    radius: f32,
    atmosphere_radius: f32,
    rotation_x: f32,
    rotation_y: f32,
    cloud_rotation_y: f32,
    camera_distance: f32,
    light_x: f32,
    light_y: f32,
    light_z: f32,
}

#[derive(Debug, Serialize)]
#[serde(rename_all = "camelCase")]
struct SurfacePatchDto {
    lat_degrees: f32,
    lon_degrees: f32,
    radius_degrees: f32,
    stretch_x: f32,
    stretch_y: f32,
}

#[derive(Debug, Serialize)]
struct RenderPayloadDto {
    state: EarthStateDto,
    patches: Vec<SurfacePatchDto>,
}

impl From<EarthRenderState> for EarthStateDto {
    fn from(state: EarthRenderState) -> Self {
        Self {
            radius: state.radius,
            atmosphere_radius: state.atmosphere_radius,
            rotation_x: state.rotation_x,
            rotation_y: state.rotation_y,
            cloud_rotation_y: state.cloud_rotation_y,
            camera_distance: state.camera_distance,
            light_x: state.light_x,
            light_y: state.light_y,
            light_z: state.light_z,
        }
    }
}

impl From<SurfacePatch> for SurfacePatchDto {
    fn from(patch: SurfacePatch) -> Self {
        Self {
            lat_degrees: patch.lat_degrees,
            lon_degrees: patch.lon_degrees,
            radius_degrees: patch.radius_degrees,
            stretch_x: patch.stretch_x,
            stretch_y: patch.stretch_y,
        }
    }
}

#[tauri::command]
fn tick(
    engine: State<'_, Mutex<Engine>>,
    input: InputDto,
    dt_seconds: f32,
) -> Result<RenderPayloadDto, String> {
    let mut engine = engine
        .lock()
        .map_err(|_| "Rust engine mutex was poisoned".to_string())?;

    engine.set_input(ControlInput {
        rotate_x: input.rotate_x,
        rotate_y: input.rotate_y,
        zoom: input.zoom,
        reset: input.reset,
    });
    engine.tick(dt_seconds);

    Ok(RenderPayloadDto {
        state: engine.render_state().into(),
        patches: engine
            .surface_patches()
            .iter()
            .copied()
            .map(SurfacePatchDto::from)
            .collect(),
    })
}

fn main() {
    tauri::Builder::default()
        .manage(Mutex::new(Engine::new()))
        .invoke_handler(tauri::generate_handler![tick])
        .run(tauri::generate_context!())
        .expect("failed to run Tauri renderer");
}

