#include "RustSimulationComponent.h"

#include <CryCore/Platform/CryLibrary.h>
#include <CryInput/IInput.h>
#include <CryRenderer/IRenderAuxGeom.h>
#include <CrySystem/ISystem.h>

namespace
{
constexpr ColorB OceanColor(26, 115, 242, 255);
constexpr ColorB LandColor(40, 131, 69, 255);
constexpr ColorB AtmosphereColor(136, 203, 255, 128);

#if defined(_WIN32)
constexpr const char* DefaultRustLibraryPath = "native/windows/rust_engine.dll";
#elif defined(__APPLE__)
constexpr const char* DefaultRustLibraryPath = "native/macos/librust_engine.dylib";
#else
constexpr const char* DefaultRustLibraryPath = "native/linux/librust_engine.so";
#endif

#ifndef RUST_ENGINE_LIBRARY_PATH
#define RUST_ENGINE_LIBRARY_PATH DefaultRustLibraryPath
#endif
}

CRustSimulationComponent::~CRustSimulationComponent()
{
    if (Engine && Destroy)
    {
        Destroy(Engine);
        Engine = nullptr;
    }

    UnloadRustEngine();
}

void CRustSimulationComponent::ReflectType(Schematyc::CTypeDesc<CRustSimulationComponent>& Desc)
{
    Desc.SetGUID("{8F140A3C-3568-4F20-9B83-6CB1003E1F48}"_cry_guid);
    Desc.SetEditorCategory("Rust");
    Desc.SetLabel("Rust Simulation Renderer");
    Desc.SetDescription("Renders the shared Rust simulation state.");
}

void CRustSimulationComponent::Initialize()
{
    LoadRustEngine();

    if (Create)
    {
        Engine = Create();
    }
}

uint64 CRustSimulationComponent::GetEventMask() const
{
    return BIT64(ENTITY_EVENT_UPDATE);
}

void CRustSimulationComponent::ProcessEvent(const SEntityEvent& Event)
{
    if (Event.event != ENTITY_EVENT_UPDATE || !Engine || !SetControlInput || !TickEngine || !RenderState || !SurfacePatches)
    {
        return;
    }

    const float DeltaSeconds = Event.fParam[0];
    SetControlInput(Engine, ReadInput());
    TickEngine(Engine, DeltaSeconds);
    RenderRustState(RenderState(Engine), SurfacePatches(Engine));
}

void CRustSimulationComponent::LoadRustEngine()
{
    LibraryHandle = CryLoadLibrary(RUST_ENGINE_LIBRARY_PATH);
    if (!LibraryHandle)
    {
        CryWarning(VALIDATOR_MODULE_GAME, VALIDATOR_ERROR, "Could not load Rust engine library: %s", RUST_ENGINE_LIBRARY_PATH);
        return;
    }

    Create = reinterpret_cast<RustEngineCreate>(CryGetProcAddress(LibraryHandle, "rust_engine_create"));
    Destroy = reinterpret_cast<RustEngineDestroy>(CryGetProcAddress(LibraryHandle, "rust_engine_destroy"));
    SetControlInput = reinterpret_cast<RustEngineSetControlInput>(CryGetProcAddress(LibraryHandle, "rust_engine_set_control_input"));
    TickEngine = reinterpret_cast<RustEngineTick>(CryGetProcAddress(LibraryHandle, "rust_engine_tick"));
    RenderState = reinterpret_cast<RustEngineRenderState>(CryGetProcAddress(LibraryHandle, "rust_engine_render_state"));
    SurfacePatches = reinterpret_cast<RustEngineSurfacePatches>(CryGetProcAddress(LibraryHandle, "rust_engine_surface_patches"));
}

void CRustSimulationComponent::UnloadRustEngine()
{
    if (LibraryHandle)
    {
        CryFreeLibrary(LibraryHandle);
        LibraryHandle = nullptr;
    }
}

ControlInput CRustSimulationComponent::ReadInput() const
{
    ControlInput Input {};

    if (!gEnv || !gEnv->pInput)
    {
        return Input;
    }

    IInput* InputSystem = gEnv->pInput;
    if (InputSystem->InputState(eKI_Up, eIS_Down))
    {
        Input.rotate_x += 1.0f;
    }

    if (InputSystem->InputState(eKI_Down, eIS_Down))
    {
        Input.rotate_x -= 1.0f;
    }

    if (InputSystem->InputState(eKI_Left, eIS_Down))
    {
        Input.rotate_y += 1.0f;
    }

    if (InputSystem->InputState(eKI_Right, eIS_Down))
    {
        Input.rotate_y -= 1.0f;
    }

    if (InputSystem->InputState(eKI_PgUp, eIS_Down))
    {
        Input.zoom += 1.0f;
    }

    if (InputSystem->InputState(eKI_PgDn, eIS_Down))
    {
        Input.zoom -= 1.0f;
    }

    Input.reset = InputSystem->InputState(eKI_R, eIS_Down) ? 1u : 0u;
    return Input;
}

void CRustSimulationComponent::RenderRustState(EarthRenderState State, SurfacePatchView Patches) const
{
    if (!gEnv || !gEnv->pRenderer)
    {
        return;
    }

    IRenderAuxGeom* Aux = gEnv->pRenderer->GetIRenderAuxGeom();
    if (!Aux)
    {
        return;
    }

    Aux->DrawSphere(Vec3(ZERO), State.radius, OceanColor);
    Aux->DrawSphere(Vec3(ZERO), State.atmosphere_radius, AtmosphereColor);

    if (!Patches.ptr || Patches.len == 0)
    {
        return;
    }

    Matrix33 Rotation = Matrix33::CreateRotationXYZ(Ang3(State.rotation_x, 0.0f, State.rotation_y));

    for (size_t Index = 0; Index < Patches.len; ++Index)
    {
        const SurfacePatch& Patch = Patches.ptr[Index];
        const float Lat = DEG2RAD(Patch.lat_degrees);
        const float Lon = DEG2RAD(Patch.lon_degrees);
        const float CosLat = cos_tpl(Lat);
        const Vec3 Normal(CosLat * sin_tpl(Lon), CosLat * cos_tpl(Lon), sin_tpl(Lat));
        Aux->DrawSphere(Rotation * Normal * 1.025f, Patch.radius_degrees / 180.0f, LandColor);
    }
}
