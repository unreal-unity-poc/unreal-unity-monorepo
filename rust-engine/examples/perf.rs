use rust_engine::{ControlInput, Engine};
use std::env;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

fn main() {
    let args: Vec<String> = env::args().collect();
    let target = value_after(&args, "--target").unwrap_or("rust-direct");
    let frames = value_after(&args, "--frames")
        .and_then(|value| value.parse::<usize>().ok())
        .unwrap_or(600);

    let mut engine = Engine::new();
    let mut samples = Vec::with_capacity(frames);

    log_event(target, "start", None, &format!("\"frames\":{}", frames));

    for frame in 0..frames {
        let input = input_for_frame(frame);
        let started = Instant::now();
        engine.set_input(input);
        engine.tick(1.0 / 60.0);
        let elapsed = started.elapsed();
        samples.push(elapsed);

        let state = engine.render_state();
        log_event(
            target,
            "frame",
            Some(frame),
            &format!(
                "\"tick_ns\":{},\"camera_distance\":{},\"rotation_x\":{},\"rotation_y\":{},\"patch_count\":{}",
                elapsed.as_nanos(),
                state.camera_distance,
                state.rotation_x,
                state.rotation_y,
                engine.surface_patches().len()
            ),
        );
    }

    log_event(target, "summary", None, &summary_json(&samples));
}

fn value_after<'a>(args: &'a [String], flag: &str) -> Option<&'a str> {
    args.windows(2)
        .find(|pair| pair[0] == flag)
        .map(|pair| pair[1].as_str())
}

fn input_for_frame(frame: usize) -> ControlInput {
    let direction = if frame % 240 < 120 { 1.0 } else { -1.0 };
    ControlInput {
        rotate_x: if frame % 180 < 90 { 0.35 } else { -0.35 },
        rotate_y: direction,
        zoom: if frame % 300 < 150 { 0.25 } else { -0.25 },
        reset: 0,
    }
}

fn summary_json(samples: &[Duration]) -> String {
    let mut nanos: Vec<u128> = samples.iter().map(Duration::as_nanos).collect();
    nanos.sort_unstable();
    let total: u128 = nanos.iter().sum();
    let avg = total as f64 / nanos.len().max(1) as f64;
    let p50 = percentile(&nanos, 0.50);
    let p95 = percentile(&nanos, 0.95);
    let max = nanos.last().copied().unwrap_or_default();

    format!(
        "\"frames\":{},\"avg_ns\":{},\"p50_ns\":{},\"p95_ns\":{},\"max_ns\":{}",
        samples.len(),
        avg,
        p50,
        p95,
        max
    )
}

fn percentile(sorted: &[u128], p: f64) -> u128 {
    if sorted.is_empty() {
        return 0;
    }

    let index = ((sorted.len() - 1) as f64 * p).round() as usize;
    sorted[index]
}

fn log_event(target: &str, phase: &str, frame: Option<usize>, fields: &str) {
    let frame_field = frame
        .map(|frame| format!(",\"frame\":{}", frame))
        .unwrap_or_default();
    println!(
        "{{\"ts_unix_ms\":{},\"target\":\"{}\",\"phase\":\"{}\"{} ,{}}}",
        now_ms(),
        target,
        phase,
        frame_field,
        fields
    );
}

fn now_ms() -> u128 {
    SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .unwrap_or_default()
        .as_millis()
}
