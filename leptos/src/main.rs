use gloo_timers::callback::Interval;
use leptos::ev;
use leptos::prelude::*;
use leptos_dom::helpers::window_event_listener;
use rust_engine::{ControlInput, Engine, EarthRenderState, SurfacePatch};
use std::cell::RefCell;
use std::collections::HashSet;
use std::rc::Rc;

fn main() {
    console_error_panic_hook::set_once();
    mount_to_body(App);
}

#[component]
fn App() -> impl IntoView {
    let engine = Rc::new(RefCell::new(Engine::new()));
    let pressed_keys = Rc::new(RefCell::new(HashSet::<String>::new()));
    let queued_wheel_zoom = Rc::new(RefCell::new(0.0_f32));
    let initial = {
        let engine = engine.borrow();
        (engine.render_state(), engine.surface_patches().to_vec())
    };
    let (frame, set_frame) = signal(initial);

    let keys_for_keydown = Rc::clone(&pressed_keys);
    let keydown_handle = window_event_listener(ev::keydown, move |event| {
        let key = event.key().to_lowercase();
        if is_control_key(&key) {
            event.prevent_default();
        }

        keys_for_keydown.borrow_mut().insert(key);
    });

    let keys_for_keyup = Rc::clone(&pressed_keys);
    let keyup_handle = window_event_listener(ev::keyup, move |event| {
        let key = event.key().to_lowercase();
        if is_control_key(&key) {
            event.prevent_default();
        }

        keys_for_keyup.borrow_mut().remove(&key);
    });

    let wheel_for_event = Rc::clone(&queued_wheel_zoom);
    let wheel_handle = window_event_listener(ev::wheel, move |event| {
        event.prevent_default();
        let direction = -event.delta_y().signum() as f32;
        if direction != 0.0 {
            *wheel_for_event.borrow_mut() += direction * 6.0;
        }
    });

    on_cleanup(move || {
        keydown_handle.remove();
        keyup_handle.remove();
        wheel_handle.remove();
    });

    let engine_for_timer = Rc::clone(&engine);
    let keys_for_timer = Rc::clone(&pressed_keys);
    let wheel_for_timer = Rc::clone(&queued_wheel_zoom);
    let interval = Interval::new(33, move || {
        let mut engine = engine_for_timer.borrow_mut();
        engine.set_input(read_input(&keys_for_timer, &wheel_for_timer));
        engine.tick(1.0 / 30.0);
        set_frame.set((engine.render_state(), engine.surface_patches().to_vec()));
    });
    interval.forget();

    view! {
        <style>{STYLE}</style>
        <main>
            {move || {
                let (state, patches) = frame.get();
                let globe_radius = 240.0 * (4.2 / state.camera_distance);
                view! {
                    <svg viewBox="0 0 900 700" width="900" height="700">
                        <defs>
                            <radialGradient id="ocean" cx="36%" cy="34%" r="70%">
                                <stop offset="0%" stop-color="#3f91e7" />
                                <stop offset="55%" stop-color="#155aad" />
                                <stop offset="100%" stop-color="#04133e" />
                            </radialGradient>
                        </defs>
                        <rect width="900" height="700" fill="#080b12" />
                        <circle cx="450" cy="350" r=globe_radius fill="url(#ocean)" stroke="#82bfff" stroke-width="2" />
                        <For
                            each=move || patches.clone()
                            key=|patch| (patch.lat_degrees.to_bits(), patch.lon_degrees.to_bits())
                            children=move |patch| view! { <SurfacePatchShape patch=patch state=state globe_radius=globe_radius /> }
                        />
                        <circle cx="450" cy="350" r=globe_radius * state.atmosphere_radius fill="none" stroke="rgba(136,203,255,0.58)" stroke-width="4" />
                    </svg>
                }
            }}
        </main>
    }
}

fn is_control_key(key: &str) -> bool {
    matches!(
        key,
        "arrowup" | "arrowdown" | "arrowleft" | "arrowright" | "=" | "+" | "pageup" | "-"
            | "pagedown" | "r"
    )
}

fn read_input(
    pressed_keys: &Rc<RefCell<HashSet<String>>>,
    queued_wheel_zoom: &Rc<RefCell<f32>>,
) -> ControlInput {
    let keys = pressed_keys.borrow();
    let mut rotate_x = 0.0;
    let mut rotate_y = 0.0;
    let mut zoom = 0.0;

    if keys.contains("arrowup") {
        rotate_x += 1.0;
    }

    if keys.contains("arrowdown") {
        rotate_x -= 1.0;
    }

    if keys.contains("arrowleft") {
        rotate_y += 1.0;
    }

    if keys.contains("arrowright") {
        rotate_y -= 1.0;
    }

    if keys.contains("=") || keys.contains("+") || keys.contains("pageup") {
        zoom += 1.0;
    }

    if keys.contains("-") || keys.contains("pagedown") {
        zoom -= 1.0;
    }

    let mut queued = queued_wheel_zoom.borrow_mut();
    let queued_zoom_for_frame = queued.clamp(-1.0, 1.0);
    zoom += queued_zoom_for_frame;
    *queued -= queued_zoom_for_frame;
    if queued.abs() < 0.01 {
        *queued = 0.0;
    }

    ControlInput {
        rotate_x,
        rotate_y,
        zoom,
        reset: u32::from(keys.contains("r")),
    }
}

#[component]
fn SurfacePatchShape(patch: SurfacePatch, state: EarthRenderState, globe_radius: f32) -> impl IntoView {
    let normal = rotate(spherical_normal(patch.lat_degrees, patch.lon_degrees), state);
    let visible = normal.2 >= -0.08;
    let x = 450.0 + normal.0 * globe_radius;
    let y = 350.0 - normal.1 * globe_radius;
    let rx = globe_radius * patch.radius_degrees / 90.0 * patch.stretch_x;
    let ry = globe_radius * patch.radius_degrees / 90.0 * patch.stretch_y;

    view! {
        <Show when=move || visible>
            <ellipse cx=x cy=y rx=rx ry=ry fill="#288345" />
        </Show>
    }
}

fn spherical_normal(lat_degrees: f32, lon_degrees: f32) -> (f32, f32, f32) {
    let lat = lat_degrees.to_radians();
    let lon = lon_degrees.to_radians();
    let cos_lat = lat.cos();
    (cos_lat * lon.sin(), lat.sin(), cos_lat * lon.cos())
}

fn rotate(mut v: (f32, f32, f32), state: EarthRenderState) -> (f32, f32, f32) {
    let (cos_x, sin_x) = (state.rotation_x.cos(), state.rotation_x.sin());
    let y = v.1 * cos_x - v.2 * sin_x;
    let z = v.1 * sin_x + v.2 * cos_x;
    v.1 = y;
    v.2 = z;

    let (cos_y, sin_y) = (state.rotation_y.cos(), state.rotation_y.sin());
    let x = v.0 * cos_y + v.2 * sin_y;
    v.2 = -v.0 * sin_y + v.2 * cos_y;
    v.0 = x;
    v
}

const STYLE: &str = r#"
html, body {
  width: 100%;
  height: 100%;
  margin: 0;
  background: #080b12;
}

main {
  width: 100%;
  height: 100%;
  display: grid;
  place-items: center;
}

svg {
  width: min(100vw, 900px);
  height: min(100vh, 700px);
}
"#;
