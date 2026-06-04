use dioxus::prelude::*;
use rust_engine::{Engine, EarthRenderState, SurfacePatch};
use std::time::Duration;

fn main() {
    dioxus::launch(App);
}

#[component]
fn App() -> Element {
    let engine = use_signal(Engine::new);
    let mut frame = use_signal(|| 0_u64);

    use_future(move || async move {
        loop {
            tokio::time::sleep(Duration::from_millis(16)).await;
            engine.write().tick(1.0 / 60.0);
            frame += 1;
        }
    });

    let _ = frame.read();
    let state = engine.read().render_state();
    let patches = engine.read().surface_patches().to_vec();
    let globe_radius = 240.0 * (4.2 / state.camera_distance);

    rsx! {
        style { {STYLE} }
        div { class: "shell",
            svg { view_box: "0 0 900 700", width: "900", height: "700",
                rect { width: "900", height: "700", fill: "#080b12" }
                defs {
                    radialGradient { id: "ocean", cx: "36%", cy: "34%", r: "70%",
                        stop { offset: "0%", stop_color: "#3f91e7" }
                        stop { offset: "55%", stop_color: "#155aad" }
                        stop { offset: "100%", stop_color: "#04133e" }
                    }
                }
                circle { cx: "450", cy: "350", r: "{globe_radius}", fill: "url(#ocean)", stroke: "#82bfff", stroke_width: "2" }
                for patch in patches {
                    SurfacePatchShape { patch, state, globe_radius }
                }
                circle { cx: "450", cy: "350", r: "{globe_radius * state.atmosphere_radius}", fill: "none", stroke: "rgba(136,203,255,0.58)", stroke_width: "4" }
            }
        }
    }
}

#[component]
fn SurfacePatchShape(patch: SurfacePatch, state: EarthRenderState, globe_radius: f32) -> Element {
    let normal = rotate(spherical_normal(patch.lat_degrees, patch.lon_degrees), state);
    if normal.2 < -0.08 {
        return rsx! {};
    }

    let x = 450.0 + normal.0 * globe_radius;
    let y = 350.0 - normal.1 * globe_radius;
    let rx = globe_radius * patch.radius_degrees / 90.0 * patch.stretch_x;
    let ry = globe_radius * patch.radius_degrees / 90.0 * patch.stretch_y;

    rsx! {
        ellipse { cx: "{x}", cy: "{y}", rx: "{rx}", ry: "{ry}", fill: "#288345" }
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
html, body, #main {
  width: 100%;
  height: 100%;
  margin: 0;
  background: #080b12;
}

.shell {
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

