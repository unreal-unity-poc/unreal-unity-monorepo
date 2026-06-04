using System;
using System.Runtime.InteropServices;
using Godot;

namespace RustGodotRenderer;

public partial class RustSimulationRenderer : Node3D
{
    private const float ZoomImpulseFrames = 6.0f;

    private IntPtr engine;
    private Node3D globeRoot = null!;
    private MeshInstance3D earthSphere = null!;
    private MeshInstance3D atmosphereSphere = null!;
    private Camera3D camera = null!;
    private DirectionalLight3D light = null!;
    private StandardMaterial3D oceanMaterial = null!;
    private StandardMaterial3D landMaterial = null!;
    private StandardMaterial3D atmosphereMaterial = null!;
    private EngineEventCallback engineEventCallback = null!;
    private GCHandle selfHandle;
    private EarthRenderState latestRustState;
    private bool hasLatestRustState;
    private float queuedWheelZoom;

    public override void _Ready()
    {
        DisplayServer.WindowSetTitle("Rust Godot Renderer");
        SetProcessInput(true);
        SetProcessUnhandledInput(true);

        oceanMaterial = new StandardMaterial3D { AlbedoColor = new Color(0.05f, 0.26f, 0.65f) };
        landMaterial = new StandardMaterial3D { AlbedoColor = new Color(0.16f, 0.47f, 0.22f) };
        atmosphereMaterial = new StandardMaterial3D
        {
            AlbedoColor = new Color(0.45f, 0.72f, 1.0f, 0.24f),
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
        };

        engine = RustEngineNative.Create();
        engineEventCallback = OnRustEngineEvent;
        selfHandle = GCHandle.Alloc(this);
        RustEngineNative.SetEventCallback(engine, engineEventCallback, GCHandle.ToIntPtr(selfHandle));

        BuildGlobe();
        BuildSurfacePatches();
        EnsureCamera();
        EnsureLight();
    }

    public override void _ExitTree()
    {
        if (engine != IntPtr.Zero)
        {
            RustEngineNative.ClearEventCallback(engine);
            RustEngineNative.Destroy(engine);
            engine = IntPtr.Zero;
        }

        if (selfHandle.IsAllocated)
        {
            selfHandle.Free();
        }
    }

    public override void _Process(double delta)
    {
        if (engine == IntPtr.Zero)
        {
            return;
        }

        RustEngineNative.SetControlInput(engine, ReadInput());
        RustEngineNative.Tick(engine, (float)delta);
        ApplyRustState(hasLatestRustState ? latestRustState : RustEngineNative.RenderState(engine));
    }

    public override void _Input(InputEvent inputEvent)
    {
        if (QueueZoomFromEvent(inputEvent))
        {
            GetViewport().SetInputAsHandled();
        }
    }

    public override void _UnhandledInput(InputEvent inputEvent)
    {
        if (QueueZoomFromEvent(inputEvent))
        {
            GetViewport().SetInputAsHandled();
        }
    }

    private bool QueueZoomFromEvent(InputEvent inputEvent)
    {
        switch (inputEvent)
        {
            case InputEventMouseButton { Pressed: true } mouseButton when mouseButton.ButtonIndex == MouseButton.WheelUp:
                queuedWheelZoom += ZoomImpulseFrames;
                return true;
            case InputEventMouseButton { Pressed: true } mouseButton when mouseButton.ButtonIndex == MouseButton.WheelDown:
                queuedWheelZoom -= ZoomImpulseFrames;
                return true;
            case InputEventMagnifyGesture magnifyGesture:
            {
                float impulse = Mathf.Clamp((magnifyGesture.Factor - 1.0f) * ZoomImpulseFrames * 2.0f, -ZoomImpulseFrames, ZoomImpulseFrames);
                if (Mathf.Abs(impulse) < 0.01f)
                {
                    return false;
                }

                queuedWheelZoom += impulse;
                return true;
            }
            case InputEventPanGesture panGesture:
            {
                float impulse = Mathf.Clamp(-panGesture.Delta.Y / 12.0f, -ZoomImpulseFrames, ZoomImpulseFrames);
                if (Mathf.Abs(impulse) < 0.01f)
                {
                    return false;
                }

                queuedWheelZoom += impulse;
                return true;
            }
            default:
                return false;
        }
    }

    private void HandleRustEngineEvent(EngineEvent engineEvent)
    {
        if (engineEvent.Kind != 1)
        {
            return;
        }

        latestRustState = engineEvent.State;
        hasLatestRustState = true;
    }

    private static void OnRustEngineEvent(IntPtr userData, EngineEvent engineEvent)
    {
        if (userData == IntPtr.Zero)
        {
            return;
        }

        GCHandle handle = GCHandle.FromIntPtr(userData);
        if (handle.Target is RustSimulationRenderer renderer)
        {
            renderer.HandleRustEngineEvent(engineEvent);
        }
    }

    private ControlInput ReadInput()
    {
        float rotateX = 0.0f;
        float rotateY = 0.0f;
        float zoom = 0.0f;

        if (Input.IsKeyPressed(Key.Up))
        {
            rotateX += 1.0f;
        }

        if (Input.IsKeyPressed(Key.Down))
        {
            rotateX -= 1.0f;
        }

        if (Input.IsKeyPressed(Key.Left))
        {
            rotateY += 1.0f;
        }

        if (Input.IsKeyPressed(Key.Right))
        {
            rotateY -= 1.0f;
        }

        if (Input.IsKeyPressed(Key.Plus) || Input.IsKeyPressed(Key.Equal) || Input.IsKeyPressed(Key.KpAdd) || Input.IsKeyPressed(Key.Pageup))
        {
            zoom += 1.0f;
        }

        if (Input.IsKeyPressed(Key.Minus) || Input.IsKeyPressed(Key.KpSubtract) || Input.IsKeyPressed(Key.Pagedown))
        {
            zoom -= 1.0f;
        }

        float queuedZoomForFrame = Mathf.Clamp(queuedWheelZoom, -1.0f, 1.0f);
        zoom += queuedZoomForFrame;
        queuedWheelZoom -= queuedZoomForFrame;
        if (Mathf.Abs(queuedWheelZoom) < 0.01f)
        {
            queuedWheelZoom = 0.0f;
        }

        return new ControlInput
        {
            RotateX = rotateX,
            RotateY = rotateY,
            Zoom = zoom,
            Reset = Input.IsKeyPressed(Key.R) ? 1u : 0u,
        };
    }

    private void BuildGlobe()
    {
        globeRoot = new Node3D { Name = "Rust Earth" };
        AddChild(globeRoot);

        earthSphere = new MeshInstance3D
        {
            Name = "Earth Ocean",
            Mesh = new SphereMesh { Radius = 1.0f, Height = 2.0f },
            MaterialOverride = oceanMaterial,
        };
        globeRoot.AddChild(earthSphere);

        atmosphereSphere = new MeshInstance3D
        {
            Name = "Atmosphere",
            Mesh = new SphereMesh { Radius = 1.0f, Height = 2.0f },
            MaterialOverride = atmosphereMaterial,
        };
        globeRoot.AddChild(atmosphereSphere);
    }

    private unsafe void BuildSurfacePatches()
    {
        SurfacePatchView view = RustEngineNative.SurfacePatches(engine);
        if (view.Ptr == IntPtr.Zero || view.Len == UIntPtr.Zero)
        {
            return;
        }

        SurfacePatch* patches = (SurfacePatch*)view.Ptr.ToPointer();
        ulong count = view.Len.ToUInt64();

        for (ulong index = 0; index < count; index++)
        {
            SurfacePatch patch = patches[index];
            MeshInstance3D land = new()
            {
                Name = $"Land Patch {index}",
                Mesh = new SphereMesh { Radius = 1.0f, Height = 2.0f },
                MaterialOverride = landMaterial,
                Position = SphericalNormal(patch.LatDegrees, patch.LonDegrees) * 1.025f,
            };

            float size = patch.RadiusDegrees / 100.0f;
            land.Scale = new Vector3(size * patch.StretchX, size * patch.StretchY, 0.025f);
            globeRoot.AddChild(land);
        }
    }

    private void ApplyRustState(EarthRenderState state)
    {
        globeRoot.Rotation = new Vector3(state.RotationX, state.RotationY, 0.0f);
        earthSphere.Scale = Vector3.One * state.Radius;
        atmosphereSphere.Scale = Vector3.One * state.AtmosphereRadius;

        camera.Position = new Vector3(0.0f, 0.0f, state.CameraDistance);
        camera.LookAt(Vector3.Zero, Vector3.Up);
        light.LookAt(new Vector3(state.LightX, state.LightY, state.LightZ), Vector3.Up);
    }

    private void EnsureCamera()
    {
        camera = new Camera3D
        {
            Name = "Main Camera",
            Current = true,
        };
        AddChild(camera);
    }

    private void EnsureLight()
    {
        light = new DirectionalLight3D
        {
            Name = "Key Light",
            LightEnergy = 1.25f,
        };
        AddChild(light);
    }

    private static Vector3 SphericalNormal(float latDegrees, float lonDegrees)
    {
        float lat = Mathf.DegToRad(latDegrees);
        float lon = Mathf.DegToRad(lonDegrees);
        float cosLat = Mathf.Cos(lat);

        return new Vector3(
            cosLat * Mathf.Sin(lon),
            Mathf.Sin(lat),
            cosLat * Mathf.Cos(lon)).Normalized();
    }
}
