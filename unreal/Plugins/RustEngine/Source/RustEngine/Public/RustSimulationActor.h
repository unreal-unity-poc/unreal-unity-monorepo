#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "rust_engine.h"
#include "RustSimulationActor.generated.h"

class UInstancedStaticMeshComponent;
class UStaticMeshComponent;

UCLASS()
class RUSTENGINE_API ARustSimulationActor : public AActor
{
    GENERATED_BODY()

public:
    ARustSimulationActor();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

private:
    using RustEngineCreate = RustEngine* (*)();
    using RustEngineDestroy = void (*)(RustEngine*);
    using RustEngineSetControlInput = void (*)(RustEngine*, ControlInput);
    using RustEngineTick = void (*)(RustEngine*, float);
    using RustEngineRenderState = EarthRenderState (*)(const RustEngine*);
    using RustEngineSurfacePatches = SurfacePatchView (*)(const RustEngine*);

    void LoadRustEngine();
    void UnloadRustEngine();
    ControlInput ReadInput() const;
    void RenderRustState(EarthRenderState State, SurfacePatchView Patches);

    void* RustLibraryHandle = nullptr;
    RustEngine* Engine = nullptr;
    RustEngineCreate Create = nullptr;
    RustEngineDestroy Destroy = nullptr;
    RustEngineSetControlInput SetControlInput = nullptr;
    RustEngineTick TickEngine = nullptr;
    RustEngineRenderState RenderState = nullptr;
    RustEngineSurfacePatches SurfacePatches = nullptr;

    UPROPERTY()
    UStaticMeshComponent* PlayerMesh = nullptr;

    UPROPERTY()
    UInstancedStaticMeshComponent* OrbitMeshes = nullptr;

    UPROPERTY()
    UStaticMeshComponent* GroundMesh = nullptr;
};
