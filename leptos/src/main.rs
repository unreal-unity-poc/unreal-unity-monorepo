use gloo_timers::callback::Interval;
use leptos::prelude::*;
use rust_engine::{Engine, EarthRenderState, SurfacePatch};
use std::cell::RefCell;
use std::rc::Rc;

fn main() {
    console_error_panic_hook::set_once();
    mount_to_body(App);
}

#[component]
fn App() -> impl IntoView {
    let engine = Rc::new(RefCell::new(Engine::new()));
    let initial = {
        let engine = engine.borrow();
        (engine.render_state(), engine.surface_patches().to_vec())
    };
    let (frame, set_frame) = signal(initial);

    let engine_for_timer = Rc::clone(&engine);
    let interval = Interval::new(16, move || {
        let mut engine = engine_for_timer.borrow_mut();
        engine.tick(1.0 / 60.0);
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

