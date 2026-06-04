#include "rust_engine.h"

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>

#include <algorithm>
#include <cmath>
#include <dlfcn.h>
#include <sstream>
#include <stdexcept>
#include <string>

using RustEngineCreate = RustEngine* (*)();
using RustEngineDestroy = void (*)(RustEngine*);
using RustEngineSetControlInput = void (*)(RustEngine*, ControlInput);
using RustEngineTick = void (*)(RustEngine*, float);
using RustEngineRenderState = EarthRenderState (*)(const RustEngine*);
using RustEngineSurfacePatches = SurfacePatchView (*)(const RustEngine*);

namespace
{
NSString* HtmlDocument()
{
    return @R"HTML(
<!doctype html>
<html lang="en">
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    html, body {
      width: 100%;
      height: 100%;
      margin: 0;
      overflow: hidden;
      background: #080b12;
    }

    canvas {
      width: 100vw;
      height: 100vh;
      display: block;
    }
  </style>
</head>
<body>
  <canvas id="scene"></canvas>
  <script>
    const canvas = document.getElementById("scene");
    const context = canvas.getContext("2d");

    function resize() {
      const scale = window.devicePixelRatio || 1;
      canvas.width = Math.floor(window.innerWidth * scale);
      canvas.height = Math.floor(window.innerHeight * scale);
      context.setTransform(scale, 0, 0, scale, 0, 0);
    }

    window.addEventListener("resize", resize);
    resize();

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
      const width = window.innerWidth;
      const height = window.innerHeight;
      const state = payload.state;
      const center = { x: width * 0.5, y: height * 0.5 };
      const globeRadius = Math.min(width, height) * 0.34 * (4.2 / state.cameraDistance);

      context.clearRect(0, 0, width, height);
      context.fillStyle = "#080b12";
      context.fillRect(0, 0, width, height);

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

      for (const patch of payload.patches) {
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

      const clouds = 10;
      context.strokeStyle = "rgba(255,255,255,0.28)";
      context.lineWidth = 4;
      for (let index = 0; index < clouds; index += 1) {
        const angle = state.cloudRotationY + index * 0.72;
        const x = center.x + Math.sin(angle) * globeRadius * 0.55;
        const y = center.y + Math.cos(angle * 1.7) * globeRadius * 0.38;
        context.beginPath();
        context.ellipse(x, y, globeRadius * 0.16, globeRadius * 0.035, angle, 0, Math.PI * 2);
        context.stroke();
      }

      context.restore();

      context.strokeStyle = "rgba(136, 203, 255, 0.58)";
      context.lineWidth = 4;
      context.beginPath();
      context.arc(center.x, center.y, globeRadius * state.atmosphereRadius, 0, Math.PI * 2);
      context.stroke();

      const shade = context.createRadialGradient(
        center.x - globeRadius * state.lightX,
        center.y + globeRadius * state.lightY,
        globeRadius * 0.1,
        center.x,
        center.y,
        globeRadius * 1.15,
      );
      shade.addColorStop(0, "rgba(255,255,255,0.0)");
      shade.addColorStop(0.62, "rgba(0,0,0,0.0)");
      shade.addColorStop(1, "rgba(0,0,0,0.52)");
      context.fillStyle = shade;
      context.beginPath();
      context.arc(center.x, center.y, globeRadius, 0, Math.PI * 2);
      context.fill();
    }

    window.renderRustState = draw;
  </script>
</body>
</html>
)HTML";
}

template <typename T>
T ResolveSymbol(void* handle, const char* name)
{
    void* symbol = dlsym(handle, name);
    if (!symbol) {
        throw std::runtime_error(std::string("Missing Rust symbol: ") + name);
    }

    return reinterpret_cast<T>(symbol);
}
}

@interface BrowserFfiDelegate : NSObject <NSApplicationDelegate, WKNavigationDelegate>
@end

@implementation BrowserFfiDelegate {
    NSWindow* _window;
    WKWebView* _webView;
    NSTimer* _timer;
    NSDate* _lastFrameDate;
    id _eventMonitor;
    NSMutableSet<NSNumber*>* _pressedKeys;
    float _scrollZoom;
    bool _pageReady;

    void* _libraryHandle;
    RustEngine* _engine;
    RustEngineCreate _create;
    RustEngineDestroy _destroy;
    RustEngineSetControlInput _setControlInput;
    RustEngineTick _tickEngine;
    RustEngineRenderState _renderState;
    RustEngineSurfacePatches _surfacePatches;
}

- (void)applicationDidFinishLaunching:(NSNotification*)notification
{
    (void)notification;
    _pressedKeys = [NSMutableSet set];

    [self loadRustEngine];
    [self createWindow];

    _lastFrameDate = [NSDate date];
    _timer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                             target:self
                                           selector:@selector(frame:)
                                           userInfo:nil
                                            repeats:YES];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender
{
    (void)sender;
    return YES;
}

- (void)applicationWillTerminate:(NSNotification*)notification
{
    (void)notification;

    if (_eventMonitor) {
        [NSEvent removeMonitor:_eventMonitor];
        _eventMonitor = nil;
    }

    [_timer invalidate];
    _timer = nil;

    if (_engine && _destroy) {
        _destroy(_engine);
        _engine = nullptr;
    }

    if (_libraryHandle) {
        dlclose(_libraryHandle);
        _libraryHandle = nullptr;
    }
}

- (void)webView:(WKWebView*)webView didFinishNavigation:(WKNavigation*)navigation
{
    (void)webView;
    (void)navigation;
    _pageReady = true;
}

- (void)loadRustEngine
{
    _libraryHandle = dlopen(RUST_ENGINE_LIBRARY_PATH, RTLD_NOW | RTLD_LOCAL);
    if (!_libraryHandle) {
        NSString* reason = [NSString stringWithUTF8String:dlerror()];
        @throw [NSException exceptionWithName:@"RustEngineLoadError"
                                       reason:reason
                                     userInfo:nil];
    }

    _create = ResolveSymbol<RustEngineCreate>(_libraryHandle, "rust_engine_create");
    _destroy = ResolveSymbol<RustEngineDestroy>(_libraryHandle, "rust_engine_destroy");
    _setControlInput = ResolveSymbol<RustEngineSetControlInput>(_libraryHandle, "rust_engine_set_control_input");
    _tickEngine = ResolveSymbol<RustEngineTick>(_libraryHandle, "rust_engine_tick");
    _renderState = ResolveSymbol<RustEngineRenderState>(_libraryHandle, "rust_engine_render_state");
    _surfacePatches = ResolveSymbol<RustEngineSurfacePatches>(_libraryHandle, "rust_engine_surface_patches");

    _engine = _create();
}

- (void)createWindow
{
    NSRect frame = NSMakeRect(0, 0, 980, 760);
    _window = [[NSWindow alloc] initWithContentRect:frame
                                         styleMask:(NSWindowStyleMaskTitled |
                                                    NSWindowStyleMaskClosable |
                                                    NSWindowStyleMaskResizable |
                                                    NSWindowStyleMaskMiniaturizable)
                                           backing:NSBackingStoreBuffered
                                             defer:NO];
    [_window setTitle:@"Rust Browser FFI Earth"];
    [_window center];

    WKWebViewConfiguration* configuration = [[WKWebViewConfiguration alloc] init];
    _webView = [[WKWebView alloc] initWithFrame:frame configuration:configuration];
    [_webView setNavigationDelegate:self];
    [_webView loadHTMLString:HtmlDocument() baseURL:nil];

    [_window setContentView:_webView];
    [_window makeKeyAndOrderFront:nil];
    [_webView.window makeFirstResponder:_webView];

    __weak BrowserFfiDelegate* weakSelf = self;
    _eventMonitor = [NSEvent addLocalMonitorForEventsMatchingMask:(NSEventMaskKeyDown |
                                                                    NSEventMaskKeyUp |
                                                                    NSEventMaskScrollWheel)
                                                          handler:^NSEvent*(NSEvent* event) {
        BrowserFfiDelegate* strongSelf = weakSelf;
        if (!strongSelf) {
            return event;
        }

        [strongSelf handleEvent:event];
        return event;
    }];
}

- (void)handleEvent:(NSEvent*)event
{
    if (event.window != _window) {
        return;
    }

    if (event.type == NSEventTypeKeyDown) {
        [_pressedKeys addObject:@(event.keyCode)];
    } else if (event.type == NSEventTypeKeyUp) {
        [_pressedKeys removeObject:@(event.keyCode)];
    } else if (event.type == NSEventTypeScrollWheel) {
        _scrollZoom += std::clamp(static_cast<float>(event.scrollingDeltaY / 18.0), -1.0f, 1.0f);
    }
}

- (void)frame:(NSTimer*)timer
{
    (void)timer;
    if (!_pageReady || !_engine) {
        return;
    }

    NSDate* now = [NSDate date];
    NSTimeInterval elapsed = [now timeIntervalSinceDate:_lastFrameDate];
    _lastFrameDate = now;

    const float dt = std::clamp(static_cast<float>(elapsed), 0.0f, 0.1f);
    _setControlInput(_engine, [self readInput]);
    _tickEngine(_engine, dt);

    const std::string json = [self buildJson];
    NSString* payload = [NSString stringWithUTF8String:json.c_str()];
    NSString* script = [NSString stringWithFormat:@"window.renderRustState(%@)", payload];
    [_webView evaluateJavaScript:script completionHandler:nil];
}

- (ControlInput)readInput
{
    ControlInput input {};

    if ([_pressedKeys containsObject:@(126)]) {
        input.rotate_x += 1.0f;
    }

    if ([_pressedKeys containsObject:@(125)]) {
        input.rotate_x -= 1.0f;
    }

    if ([_pressedKeys containsObject:@(123)]) {
        input.rotate_y += 1.0f;
    }

    if ([_pressedKeys containsObject:@(124)]) {
        input.rotate_y -= 1.0f;
    }

    if ([_pressedKeys containsObject:@(24)] || [_pressedKeys containsObject:@(69)] || [_pressedKeys containsObject:@(116)]) {
        input.zoom += 1.0f;
    }

    if ([_pressedKeys containsObject:@(27)] || [_pressedKeys containsObject:@(78)] || [_pressedKeys containsObject:@(121)]) {
        input.zoom -= 1.0f;
    }

    if ([_pressedKeys containsObject:@(15)]) {
        input.reset = 1;
    }

    input.zoom += std::clamp(_scrollZoom, -1.0f, 1.0f);
    _scrollZoom = 0.0f;

    return input;
}

- (std::string)buildJson
{
    const EarthRenderState state = _renderState(_engine);
    const SurfacePatchView patches = _surfacePatches(_engine);

    std::ostringstream json;
    json << "{\"state\":{"
         << "\"radius\":" << state.radius
         << ",\"atmosphereRadius\":" << state.atmosphere_radius
         << ",\"rotationX\":" << state.rotation_x
         << ",\"rotationY\":" << state.rotation_y
         << ",\"cloudRotationY\":" << state.cloud_rotation_y
         << ",\"cameraDistance\":" << state.camera_distance
         << ",\"lightX\":" << state.light_x
         << ",\"lightY\":" << state.light_y
         << ",\"lightZ\":" << state.light_z
         << "},\"patches\":[";

    for (size_t index = 0; index < patches.len; ++index) {
        const SurfacePatch& patch = patches.ptr[index];
        if (index != 0) {
            json << ',';
        }

        json << "{\"latDegrees\":" << patch.lat_degrees
             << ",\"lonDegrees\":" << patch.lon_degrees
             << ",\"radiusDegrees\":" << patch.radius_degrees
             << ",\"stretchX\":" << patch.stretch_x
             << ",\"stretchY\":" << patch.stretch_y
             << '}';
    }

    json << "]}";
    return json.str();
}

@end

int main(int argc, const char* argv[])
{
    (void)argc;
    (void)argv;

    @autoreleasepool {
        NSApplication* app = [NSApplication sharedApplication];
        [app setActivationPolicy:NSApplicationActivationPolicyRegular];

        BrowserFfiDelegate* delegate = [[BrowserFfiDelegate alloc] init];
        [app setDelegate:delegate];
        [app activateIgnoringOtherApps:YES];
        [app run];
    }

    return 0;
}

