#pragma once

#include "rust_engine.h"

#include <CryEntitySystem/IEntityComponent.h>
#include <CrySchematyc/Reflection/TypeDesc.h>

class CRustSimulationComponent final : public IEntityComponent
{
public:
    virtual ~CRustSimulationComponent() override;

    static void ReflectType(Schematyc::CTypeDesc<CRustSimulationComponent>& Desc);

    virtual void Initialize() override;
    virtual uint64 GetEventMask() const override;
    virtual void ProcessEvent(const SEntityEvent& Event) override;

private:
    using RustEngineCreate = RustEngine* (*)();
    using RustEngineDestroy = void (*)(RustEngine*);
    using RustEngineSetControlInput = void (*)(RustEngine*, ControlInput);
    using RustEngineTick = void (*)(RustEngine*, float);
    using RustEngineSetEventCallback = void (*)(RustEngine*, RustEngineEventCallback, void*);
    using RustEngineClearEventCallback = void (*)(RustEngine*);
    using RustEngineRenderState = EarthRenderState (*)(const RustEngine*);
    using RustEngineSurfacePatches = SurfacePatchView (*)(const RustEngine*);

    static void OnRustEngineEvent(void* UserData, EngineEvent Event);

    void LoadRustEngine();
    void UnloadRustEngine();
    ControlInput ReadInput() const;
    void HandleRustEngineEvent(EngineEvent Event);
    void RenderRustState(EarthRenderState State, SurfacePatchView Patches) const;

    void* LibraryHandle = nullptr;
    RustEngine* Engine = nullptr;
    RustEngineCreate Create = nullptr;
    RustEngineDestroy Destroy = nullptr;
    RustEngineSetControlInput SetControlInput = nullptr;
    RustEngineTick TickEngine = nullptr;
    RustEngineSetEventCallback SetEventCallback = nullptr;
    RustEngineClearEventCallback ClearEventCallback = nullptr;
    RustEngineRenderState RenderState = nullptr;
    RustEngineSurfacePatches SurfacePatches = nullptr;
    EarthRenderState LatestState {};
    bool HasLatestState = false;
};
