use std::f32::consts::TAU;
use std::ffi::c_void;
use std::ptr;

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct ControlInput {
    pub rotate_x: f32,
    pub rotate_y: f32,
    pub zoom: f32,
    pub reset: u32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct EarthRenderState {
    pub radius: f32,
    pub atmosphere_radius: f32,
    pub rotation_x: f32,
    pub rotation_y: f32,
    pub cloud_rotation_y: f32,
    pub camera_distance: f32,
    pub light_x: f32,
    pub light_y: f32,
    pub light_z: f32,
}

impl Default for EarthRenderState {
    fn default() -> Self {
        Self {
            radius: 1.0,
            atmosphere_radius: 1.035,
            rotation_x: -0.25,
            rotation_y: 0.0,
            cloud_rotation_y: 0.0,
            camera_distance: 4.2,
            light_x: -0.35,
            light_y: 0.45,
            light_z: -0.82,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct SurfacePatch {
    pub lat_degrees: f32,
    pub lon_degrees: f32,
    pub radius_degrees: f32,
    pub stretch_x: f32,
    pub stretch_y: f32,
}

#[repr(C)]
#[derive(Clone, Copy, Debug)]
pub struct SurfacePatchView {
    pub ptr: *const SurfacePatch,
    pub len: usize,
}

impl Default for SurfacePatchView {
    fn default() -> Self {
        Self {
            ptr: ptr::null(),
            len: 0,
        }
    }
}

#[repr(C)]
#[derive(Clone, Copy, Debug, Default, PartialEq)]
pub struct EngineEvent {
    pub kind: u32,
    pub frame_index: u64,
    pub state: EarthRenderState,
}

pub type EngineEventCallback = unsafe extern "C" fn(user_data: *mut c_void, event: EngineEvent);

#[derive(Clone, Copy)]
struct CallbackRegistration {
    callback: EngineEventCallback,
    user_data: *mut c_void,
}

const SURFACE_PATCHES: [SurfacePatch; 12] = [
    SurfacePatch {
        lat_degrees: 46.0,
        lon_degrees: -102.0,
        radius_degrees: 18.0,
        stretch_x: 1.45,
        stretch_y: 0.85,
    },
    SurfacePatch {
        lat_degrees: 5.0,
        lon_degrees: -60.0,
        radius_degrees: 23.0,
        stretch_x: 0.95,
        stretch_y: 1.35,
    },
    SurfacePatch {
        lat_degrees: -16.0,
        lon_degrees: -58.0,
        radius_degrees: 18.0,
        stretch_x: 0.8,
        stretch_y: 1.65,
    },
    SurfacePatch {
        lat_degrees: 50.0,
        lon_degrees: 18.0,
        radius_degrees: 19.0,
        stretch_x: 1.8,
        stretch_y: 0.7,
    },
    SurfacePatch {
        lat_degrees: 21.0,
        lon_degrees: 22.0,
        radius_degrees: 20.0,
        stretch_x: 0.9,
        stretch_y: 1.5,
    },
    SurfacePatch {
        lat_degrees: 58.0,
        lon_degrees: 92.0,
        radius_degrees: 21.0,
        stretch_x: 2.1,
        stretch_y: 0.75,
    },
    SurfacePatch {
        lat_degrees: 28.0,
        lon_degrees: 79.0,
        radius_degrees: 15.0,
        stretch_x: 1.1,
        stretch_y: 0.9,
    },
    SurfacePatch {
        lat_degrees: -26.0,
        lon_degrees: 134.0,
        radius_degrees: 16.0,
        stretch_x: 1.7,
        stretch_y: 0.95,
    },
    SurfacePatch {
        lat_degrees: -78.0,
        lon_degrees: 0.0,
        radius_degrees: 14.0,
        stretch_x: 3.0,
        stretch_y: 0.65,
    },
    SurfacePatch {
        lat_degrees: 72.0,
        lon_degrees: -42.0,
        radius_degrees: 11.0,
        stretch_x: 1.4,
        stretch_y: 0.9,
    },
    SurfacePatch {
        lat_degrees: 13.0,
        lon_degrees: 102.0,
        radius_degrees: 12.0,
        stretch_x: 1.25,
        stretch_y: 0.8,
    },
    SurfacePatch {
        lat_degrees: -3.0,
        lon_degrees: 25.0,
        radius_degrees: 13.0,
        stretch_x: 1.0,
        stretch_y: 1.25,
    },
];

pub struct Engine {
    time: f32,
    frame_index: u64,
    input: ControlInput,
    state: EarthRenderState,
    callback: Option<CallbackRegistration>,
}

impl Engine {
    pub fn new() -> Self {
        Self {
            time: 0.0,
            frame_index: 0,
            input: ControlInput::default(),
            state: EarthRenderState::default(),
            callback: None,
        }
    }

    pub fn set_input(&mut self, input: ControlInput) {
        self.input = ControlInput {
            rotate_x: input.rotate_x.clamp(-1.0, 1.0),
            rotate_y: input.rotate_y.clamp(-1.0, 1.0),
            zoom: input.zoom.clamp(-1.0, 1.0),
            reset: u32::from(input.reset != 0),
        };
    }

    pub fn tick(&mut self, dt_seconds: f32) {
        let dt = dt_seconds.clamp(0.0, 0.1);
        self.time += dt;

        if self.input.reset != 0 {
            self.state = EarthRenderState::default();
        } else {
            let angular_speed = 1.9;
            self.state.rotation_x = (self.state.rotation_x
                + self.input.rotate_x * angular_speed * dt)
                .clamp(-1.35, 1.35);
            self.state.rotation_y =
                wrap_radians(self.state.rotation_y + self.input.rotate_y * angular_speed * dt);
            self.state.cloud_rotation_y = wrap_radians(self.time * 0.08);

            let zoom_speed = 2.2;
            let zoom_multiplier = (-self.input.zoom * zoom_speed * dt).exp();
            self.state.camera_distance =
                (self.state.camera_distance * zoom_multiplier).clamp(2.15, 9.5);
        }

        self.frame_index = self.frame_index.wrapping_add(1);
        self.emit_event(1);
    }

    pub fn render_state(&self) -> EarthRenderState {
        self.state
    }

    pub fn surface_patch_view(&self) -> SurfacePatchView {
        SurfacePatchView {
            ptr: SURFACE_PATCHES.as_ptr(),
            len: SURFACE_PATCHES.len(),
        }
    }

    pub fn surface_patches(&self) -> &'static [SurfacePatch] {
        &SURFACE_PATCHES
    }

    pub fn set_event_callback(
        &mut self,
        callback: Option<EngineEventCallback>,
        user_data: *mut c_void,
    ) {
        self.callback = callback.map(|callback| CallbackRegistration {
            callback,
            user_data,
        });
    }

    pub fn clear_event_callback(&mut self) {
        self.callback = None;
    }

    fn emit_event(&self, kind: u32) {
        if let Some(registration) = self.callback {
            let event = EngineEvent {
                kind,
                frame_index: self.frame_index,
                state: self.state,
            };

            unsafe {
                (registration.callback)(registration.user_data, event);
            }
        }
    }
}

impl Default for Engine {
    fn default() -> Self {
        Self::new()
    }
}

fn wrap_radians(value: f32) -> f32 {
    value.rem_euclid(TAU)
}

#[no_mangle]
pub extern "C" fn rust_engine_create() -> *mut Engine {
    Box::into_raw(Box::new(Engine::new()))
}

#[no_mangle]
pub unsafe extern "C" fn rust_engine_destroy(engine: *mut Engine) {
    if !engine.is_null() {
        drop(Box::from_raw(engine));
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_engine_set_control_input(engine: *mut Engine, input: ControlInput) {
    if let Some(engine) = engine.as_mut() {
        engine.set_input(input);
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_engine_tick(engine: *mut Engine, dt_seconds: f32) {
    if let Some(engine) = engine.as_mut() {
        engine.tick(dt_seconds);
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_engine_set_event_callback(
    engine: *mut Engine,
    callback: Option<EngineEventCallback>,
    user_data: *mut c_void,
) {
    if let Some(engine) = engine.as_mut() {
        engine.set_event_callback(callback, user_data);
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_engine_clear_event_callback(engine: *mut Engine) {
    if let Some(engine) = engine.as_mut() {
        engine.clear_event_callback();
    }
}

#[no_mangle]
pub unsafe extern "C" fn rust_engine_render_state(engine: *const Engine) -> EarthRenderState {
    engine
        .as_ref()
        .map(Engine::render_state)
        .unwrap_or_else(EarthRenderState::default)
}

#[no_mangle]
pub unsafe extern "C" fn rust_engine_surface_patches(engine: *const Engine) -> SurfacePatchView {
    engine
        .as_ref()
        .map(Engine::surface_patch_view)
        .unwrap_or_else(SurfacePatchView::default)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicU64, Ordering};

    static CALLBACK_COUNT: AtomicU64 = AtomicU64::new(0);

    unsafe extern "C" fn count_callback(_user_data: *mut c_void, event: EngineEvent) {
        if event.kind == 1 {
            CALLBACK_COUNT.fetch_add(event.frame_index, Ordering::SeqCst);
        }
    }

    #[test]
    fn creates_default_earth_state() {
        let engine = Engine::new();
        let state = engine.render_state();

        assert_eq!(state.radius, 1.0);
        assert!(state.camera_distance > state.radius);
        assert_eq!(engine.surface_patches().len(), 12);
    }

    #[test]
    fn controls_update_rust_owned_rotation_and_zoom() {
        let mut engine = Engine::new();
        let original = engine.render_state();
        engine.set_input(ControlInput {
            rotate_x: 1.0,
            rotate_y: 1.0,
            zoom: 1.0,
            reset: 0,
        });
        engine.tick(1.0 / 60.0);

        let updated = engine.render_state();
        assert!(updated.rotation_x > original.rotation_x);
        assert!(updated.rotation_y > original.rotation_y);
        assert!(updated.camera_distance < original.camera_distance);
    }

    #[test]
    fn tick_emits_callback_to_host() {
        CALLBACK_COUNT.store(0, Ordering::SeqCst);

        let mut engine = Engine::new();
        engine.set_event_callback(Some(count_callback), ptr::null_mut());
        engine.tick(1.0 / 60.0);
        engine.tick(1.0 / 60.0);

        assert_eq!(CALLBACK_COUNT.load(Ordering::SeqCst), 3);
    }
}
