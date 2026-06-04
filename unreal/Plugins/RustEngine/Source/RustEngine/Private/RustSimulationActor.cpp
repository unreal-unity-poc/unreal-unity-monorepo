#include "RustSimulationActor.h"

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/PlayerController.h"
#include "HAL/PlatformProcess.h"
#include "InputCoreTypes.h"
#include "UObject/ConstructorHelpers.h"

ARustSimulationActor::ARustSimulationActor()
{
    PrimaryActorTick.bCanEverTick = true;

    USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(SceneRoot);

    PlayerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Rust Player"));
    PlayerMesh->SetupAttachment(SceneRoot);

    OrbitMeshes = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Rust Land Patches"));
    OrbitMeshes->SetupAttachment(SceneRoot);

    GroundMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Ground"));
    GroundMesh->SetupAttachment(SceneRoot);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
        TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(
        TEXT("/Engine/BasicShapes/Plane.Plane"));

    if (SphereMesh.Succeeded())
    {
        PlayerMesh->SetStaticMesh(SphereMesh.Object);
        OrbitMeshes->SetStaticMesh(SphereMesh.Object);
    }

    if (PlaneMesh.Succeeded())
    {
        GroundMesh->SetStaticMesh(PlaneMesh.Object);
        GroundMesh->SetRelativeScale3D(FVector(30.0f, 30.0f, 1.0f));
    }
}

void ARustSimulationActor::BeginPlay()
{
    Super::BeginPlay();

    LoadRustEngine();
    if (Create)
    {
        Engine = Create();
        if (Engine && SetEventCallback)
        {
            SetEventCallback(Engine, &ARustSimulationActor::OnRustEngineEvent, this);
        }
    }
}

void ARustSimulationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Engine && Destroy)
    {
        if (ClearEventCallback)
        {
            ClearEventCallback(Engine);
        }
        Destroy(Engine);
        Engine = nullptr;
    }

    UnloadRustEngine();
    Super::EndPlay(EndPlayReason);
}

void ARustSimulationActor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (!Engine || !SetControlInput || !TickEngine || !RenderState || !SurfacePatches)
    {
        return;
    }

    SetControlInput(Engine, ReadInput());
    TickEngine(Engine, DeltaSeconds);
    RenderRustState(bHasLatestState ? LatestState : RenderState(Engine), SurfacePatches(Engine));
}

void ARustSimulationActor::LoadRustEngine()
{
    RustLibraryHandle = FPlatformProcess::GetDllHandle(RUST_ENGINE_LIBRARY_PATH);
    if (!RustLibraryHandle)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not load Rust engine library: %s"), RUST_ENGINE_LIBRARY_PATH);
        return;
    }

    Create = reinterpret_cast<RustEngineCreate>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_create")));
    Destroy = reinterpret_cast<RustEngineDestroy>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_destroy")));
    SetControlInput = reinterpret_cast<RustEngineSetControlInput>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_set_control_input")));
    TickEngine = reinterpret_cast<RustEngineTick>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_tick")));
    SetEventCallback = reinterpret_cast<RustEngineSetEventCallback>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_set_event_callback")));
    ClearEventCallback = reinterpret_cast<RustEngineClearEventCallback>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_clear_event_callback")));
    RenderState = reinterpret_cast<RustEngineRenderState>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_render_state")));
    SurfacePatches = reinterpret_cast<RustEngineSurfacePatches>(
        FPlatformProcess::GetDllExport(RustLibraryHandle, TEXT("rust_engine_surface_patches")));
}

void ARustSimulationActor::OnRustEngineEvent(void* UserData, EngineEvent Event)
{
    ARustSimulationActor* Actor = static_cast<ARustSimulationActor*>(UserData);
    if (Actor)
    {
        Actor->HandleRustEngineEvent(Event);
    }
}

void ARustSimulationActor::HandleRustEngineEvent(EngineEvent Event)
{
    if (Event.kind != 1)
    {
        return;
    }

    LatestState = Event.state;
    bHasLatestState = true;
}

void ARustSimulationActor::UnloadRustEngine()
{
    if (RustLibraryHandle)
    {
        FPlatformProcess::FreeDllHandle(RustLibraryHandle);
        RustLibraryHandle = nullptr;
    }
}

ControlInput ARustSimulationActor::ReadInput() const
{
    ControlInput Input {};
    const UWorld* World = GetWorld();
    const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;

    if (!Controller)
    {
        return Input;
    }

    if (Controller->IsInputKeyDown(EKeys::Up))
    {
        Input.rotate_x += 1.0f;
    }

    if (Controller->IsInputKeyDown(EKeys::Down))
    {
        Input.rotate_x -= 1.0f;
    }

    if (Controller->IsInputKeyDown(EKeys::Left))
    {
        Input.rotate_y += 1.0f;
    }

    if (Controller->IsInputKeyDown(EKeys::Right))
    {
        Input.rotate_y -= 1.0f;
    }

    if (Controller->IsInputKeyDown(EKeys::Equals) || Controller->IsInputKeyDown(EKeys::PageUp))
    {
        Input.zoom += 1.0f;
    }

    if (Controller->IsInputKeyDown(EKeys::Hyphen) || Controller->IsInputKeyDown(EKeys::PageDown))
    {
        Input.zoom -= 1.0f;
    }

    Input.reset = Controller->IsInputKeyDown(EKeys::R) ? 1u : 0u;
    return Input;
}

void ARustSimulationActor::RenderRustState(EarthRenderState State, SurfacePatchView Patches)
{
    OrbitMeshes->ClearInstances();

    const FRotator Rotation(
        FMath::RadiansToDegrees(State.rotation_x),
        FMath::RadiansToDegrees(State.rotation_y),
        0.0f);
    PlayerMesh->SetWorldTransform(FTransform(Rotation, FVector::ZeroVector, FVector(State.radius * 2.0f)));

    if (!Patches.ptr || Patches.len == 0)
    {
        return;
    }

    for (size_t Index = 0; Index < Patches.len; ++Index)
    {
        const SurfacePatch& Patch = Patches.ptr[Index];
        const float Lat = FMath::DegreesToRadians(Patch.lat_degrees);
        const float Lon = FMath::DegreesToRadians(Patch.lon_degrees);
        const float CosLat = FMath::Cos(Lat);
        const FVector Normal(CosLat * FMath::Sin(Lon), CosLat * FMath::Cos(Lon), FMath::Sin(Lat));
        const FVector Position = Rotation.RotateVector(Normal) * 102.0f;
        const FVector Scale(Patch.radius_degrees / 90.0f * Patch.stretch_x, Patch.radius_degrees / 90.0f * Patch.stretch_y, 0.025f);
        OrbitMeshes->AddInstance(FTransform(FRotator::ZeroRotator, Position, Scale), true);
    }
}
