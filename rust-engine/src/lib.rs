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

/// Host callback invoked only after Rust releases its mutable borrow of the engine.
///
/// Callers may read or mutate the same engine from the callback when they have
/// arranged serialized access, but they must not unwind across the C ABI.
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
    #[must_use]
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
            rotate_x: normalized_axis(input.rotate_x),
            rotate_y: normalized_axis(input.rotate_y),
            zoom: normalized_axis(input.zoom),
            reset: u32::from(input.reset != 0),
        };
    }

    /// Advances Rust-owned simulation state without invoking an external callback.
    ///
    /// The C ABI wrapper snapshots the callback registration, releases this
    /// mutable borrow, and invokes the callback afterward. Keeping callback
    /// delivery outside this method prevents reentrant hosts from creating an
    /// overlapping mutable Rust reference to the engine.
    #[must_use]
    pub fn tick(&mut self, dt_seconds: f32) -> EngineEvent {
        let dt = finite_or_zero(dt_seconds).clamp(0.0, 0.1);
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
        EngineEvent {
            kind: 1,
            frame_index: self.frame_index,
            state: self.state,
        }
    }

    #[must_use]
    pub const fn render_state(&self) -> EarthRenderState {
        self.state
    }

    #[must_use]
    pub fn surface_patch_view(&self) -> SurfacePatchView {
        SurfacePatchView {
            ptr: SURFACE_PATCHES.as_ptr(),
            len: SURFACE_PATCHES.len(),
        }
    }

    #[must_use]
    pub const fn surface_patches(&self) -> &'static [SurfacePatch] {
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

    const fn callback_registration(&self) -> Option<CallbackRegistration> {
        self.callback
    }
}

impl Default for Engine {
    fn default() -> Self {
        Self::new()
    }
}

fn finite_or_zero(value: f32) -> f32 {
    if value.is_finite() {
        value
    } else {
        0.0
    }
}

fn normalized_axis(value: f32) -> f32 {
    finite_or_zero(value).clamp(-1.0, 1.0)
}

fn wrap_radians(value: f32) -> f32 {
    value.rem_euclid(TAU)
}

#[must_use]
#[no_mangle]
pub extern "C" fn rust_engine_create() -> *mut Engine {
    Box::into_raw(Box::new(Engine::new()))
}

/// Destroys an engine allocated by [`rust_engine_create`].
///
/// # Safety
///
/// `engine` must be null or a live pointer returned by `rust_engine_create`.
/// The pointer must not be used after this call and this function must not race
/// with any other operation on the same engine.
#[no_mangle]
pub unsafe extern "C" fn rust_engine_destroy(engine: *mut Engine) {
    if !engine.is_null() {
        // SAFETY: The caller contract requires a unique live allocation created
        // by rust_engine_create, and the null case was excluded above.
        unsafe {
            drop(Box::from_raw(engine));
        }
    }
}

/// Replaces the current bounded control input.
///
/// # Safety
///
/// `engine` must be null or point to a live engine with exclusive serialized
/// access for the duration of this call.
#[no_mangle]
pub unsafe extern "C" fn rust_engine_set_control_input(engine: *mut Engine, input: ControlInput) {
    // SAFETY: The caller contract requires exclusive serialized access.
    if let Some(engine) = unsafe { engine.as_mut() } {
        engine.set_input(input);
    }
}

/// Advances one simulation frame and then invokes the registered callback.
///
/// The callback is invoked only after the mutable engine borrow is released,
/// which permits a serialized callback to reenter the C API without aliasing a
/// live Rust reference.
///
/// # Safety
///
/// `engine` must be null or point to a live engine with exclusive serialized
/// access while state is advanced. A registered callback must obey its C ABI,
/// must not unwind, and must arrange serialization before reentering.
#[no_mangle]
pub unsafe extern "C" fn rust_engine_tick(engine: *mut Engine, dt_seconds: f32) {
    let pending = {
        // SAFETY: The caller contract requires exclusive serialized access for
        // the duration of state advancement.
        let Some(engine) = (unsafe { engine.as_mut() }) else {
            return;
        };
        let event = engine.tick(dt_seconds);
        engine
            .callback_registration()
            .map(|registration| (registration, event))
    };

    if let Some((registration, event)) = pending {
        // SAFETY: The mutable engine reference ended before this call. The host
        // supplied the callback and user data under the documented C contract.
        unsafe {
            (registration.callback)(registration.user_data, event);
        }
    }
}

/// Registers a host callback and opaque host-owned user-data pointer.
///
/// # Safety
///
/// `engine` must be null or point to a live engine with exclusive serialized
/// access. The callback and user-data pointer must remain valid until cleared,
/// replaced, or the engine is destroyed.
#[no_mangle]
pub unsafe extern "C" fn rust_engine_set_event_callback(
    engine: *mut Engine,
    callback: Option<EngineEventCallback>,
    user_data: *mut c_void,
) {
    // SAFETY: The caller contract requires exclusive serialized access.
    if let Some(engine) = unsafe { engine.as_mut() } {
        engine.set_event_callback(callback, user_data);
    }
}

/// Clears the current host callback.
///
/// # Safety
///
/// `engine` must be null or point to a live engine with exclusive serialized
/// access.
#[no_mangle]
pub unsafe extern "C" fn rust_engine_clear_event_callback(engine: *mut Engine) {
    // SAFETY: The caller contract requires exclusive serialized access.
    if let Some(engine) = unsafe { engine.as_mut() } {
        engine.clear_event_callback();
    }
}

/// Returns a copy of the current render state.
///
/// # Safety
///
/// `engine` must be null or point to a live engine. The caller must prevent a
/// concurrent mutation while this function reads the state.
#[must_use]
#[no_mangle]
pub unsafe extern "C" fn rust_engine_render_state(engine: *const Engine) -> EarthRenderState {
    // SAFETY: The caller contract requires a live pointer and no concurrent
    // mutation. Null is accepted and mapped to the default state.
    unsafe { engine.as_ref() }
        .map(Engine::render_state)
        .unwrap_or_default()
}

/// Returns a borrowed view over immutable process-lifetime surface patches.
///
/// # Safety
///
/// `engine` must be null or point to a live engine. The caller must prevent a
/// concurrent mutation while this function reads the engine.
#[must_use]
#[no_mangle]
pub unsafe extern "C" fn rust_engine_surface_patches(engine: *const Engine) -> SurfacePatchView {
    // SAFETY: The caller contract requires a live pointer and no concurrent
    // mutation. Null is accepted and mapped to an empty view.
    unsafe { engine.as_ref() }
        .map(Engine::surface_patch_view)
        .unwrap_or_default()
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::atomic::{AtomicBool, AtomicU64, Ordering};

    static CALLBACK_FRAME_SUM: AtomicU64 = AtomicU64::new(0);
    static REENTRANT_READ_WAS_FINITE: AtomicBool = AtomicBool::new(false);

    unsafe extern "C" fn count_callback(_user_data: *mut c_void, event: EngineEvent) {
        if event.kind == 1 {
            CALLBACK_FRAME_SUM.fetch_add(event.frame_index, Ordering::SeqCst);
        }
    }

    unsafe extern "C" fn reentrant_read_callback(user_data: *mut c_void, _event: EngineEvent) {
        let engine = user_data.cast::<Engine>().cast_const();
        // SAFETY: The FFI tick wrapper releases its mutable borrow before this
        // callback. The test is single-threaded and the engine remains live.
        let state = unsafe { rust_engine_render_state(engine) };
        REENTRANT_READ_WAS_FINITE.store(
            state.rotation_x.is_finite() && state.camera_distance.is_finite(),
            Ordering::SeqCst,
        );
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
        let event = engine.tick(1.0 / 60.0);

        let updated = engine.render_state();
        assert_eq!(event.state, updated);
        assert!(updated.rotation_x > original.rotation_x);
        assert!(updated.rotation_y > original.rotation_y);
        assert!(updated.camera_distance < original.camera_distance);
    }

    #[test]
    fn non_finite_host_values_cannot_poison_simulation_state() {
        let mut engine = Engine::new();
        engine.set_input(ControlInput {
            rotate_x: f32::NAN,
            rotate_y: f32::INFINITY,
            zoom: f32::NEG_INFINITY,
            reset: 0,
        });
        let event = engine.tick(f32::NAN);

        for value in [
            event.state.rotation_x,
            event.state.rotation_y,
            event.state.cloud_rotation_y,
            event.state.camera_distance,
        ] {
            assert!(value.is_finite());
        }
        assert_eq!(event.state.rotation_x, EarthRenderState::default().rotation_x);
    }

    #[test]
    fn ffi_tick_emits_callback_after_state_advance() {
        CALLBACK_FRAME_SUM.store(0, Ordering::SeqCst);
        let engine = rust_engine_create();

        // SAFETY: This test owns the engine pointer and serializes all calls.
        unsafe {
            rust_engine_set_event_callback(engine, Some(count_callback), ptr::null_mut());
            rust_engine_tick(engine, 1.0 / 60.0);
            rust_engine_tick(engine, 1.0 / 60.0);
            rust_engine_destroy(engine);
        }

        assert_eq!(CALLBACK_FRAME_SUM.load(Ordering::SeqCst), 3);
    }

    #[test]
    fn ffi_callback_can_reenter_for_a_serialized_read() {
        REENTRANT_READ_WAS_FINITE.store(false, Ordering::SeqCst);
        let engine = rust_engine_create();

        // SAFETY: This test owns the engine pointer, keeps it live through the
        // callback, and serializes all calls on one thread.
        unsafe {
            rust_engine_set_event_callback(
                engine,
                Some(reentrant_read_callback),
                engine.cast::<c_void>(),
            );
            rust_engine_tick(engine, 1.0 / 60.0);
            rust_engine_destroy(engine);
        }

        assert!(REENTRANT_READ_WAS_FINITE.load(Ordering::SeqCst));
    }
}
