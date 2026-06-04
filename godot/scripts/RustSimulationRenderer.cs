using System;
using Godot;

namespace RustGodotRenderer;

public partial class RustSimulationRenderer : Node3D
{
    private IntPtr engine;
    private Node3D globeRoot = null!;
    private MeshInstance3D earthSphere = null!;
    private MeshInstance3D atmosphereSphere = null!;
    private Camera3D camera = null!;
    private DirectionalLight3D light = null!;
    private StandardMaterial3D oceanMaterial = null!;
    private StandardMaterial3D landMaterial = null!;
    private StandardMaterial3D atmosphereMaterial = null!;

    public override void _Ready()
    {
        oceanMaterial = new StandardMaterial3D { AlbedoColor = new Color(0.05f, 0.26f, 0.65f) };
        landMaterial = new StandardMaterial3D { AlbedoColor = new Color(0.16f, 0.47f, 0.22f) };
        atmosphereMaterial = new StandardMaterial3D
        {
            AlbedoColor = new Color(0.45f, 0.72f, 1.0f, 0.24f),
            Transparency = BaseMaterial3D.TransparencyEnum.Alpha,
        };

        engine = RustEngineNative.Create();

        BuildGlobe();
        BuildSurfacePatches();
        EnsureCamera();
        EnsureLight();
    }

    public override void _ExitTree()
    {
        if (engine != IntPtr.Zero)
        {
            RustEngineNative.Destroy(engine);
            engine = IntPtr.Zero;
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
        ApplyRustState(RustEngineNative.RenderState(engine));
    }

    private static ControlInput ReadInput()
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

        if (Input.IsKeyPressed(Key.Equal) || Input.IsKeyPressed(Key.KpAdd) || Input.IsKeyPressed(Key.Pageup))
        {
            zoom += 1.0f;
        }

        if (Input.IsKeyPressed(Key.Minus) || Input.IsKeyPressed(Key.KpSubtract) || Input.IsKeyPressed(Key.Pagedown))
        {
            zoom -= 1.0f;
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

