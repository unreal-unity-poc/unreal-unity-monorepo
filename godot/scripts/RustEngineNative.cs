using System;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using Godot;

namespace RustGodotRenderer;

[StructLayout(LayoutKind.Sequential)]
public struct ControlInput
{
    public float RotateX;
    public float RotateY;
    public float Zoom;
    public uint Reset;
}

[StructLayout(LayoutKind.Sequential)]
public struct EarthRenderState
{
    public float Radius;
    public float AtmosphereRadius;
    public float RotationX;
    public float RotationY;
    public float CloudRotationY;
    public float CameraDistance;
    public float LightX;
    public float LightY;
    public float LightZ;
}

[StructLayout(LayoutKind.Sequential)]
public struct SurfacePatch
{
    public float LatDegrees;
    public float LonDegrees;
    public float RadiusDegrees;
    public float StretchX;
    public float StretchY;
}

[StructLayout(LayoutKind.Sequential)]
public struct SurfacePatchView
{
    public IntPtr Ptr;
    public UIntPtr Len;
}

internal static class RustEngineNative
{
    private const string LibraryName = "rust_engine";

    static RustEngineNative()
    {
        NativeLibrary.SetDllImportResolver(typeof(RustEngineNative).Assembly, ResolveLibrary);
    }

    [DllImport(LibraryName, EntryPoint = "rust_engine_create")]
    internal static extern IntPtr Create();

    [DllImport(LibraryName, EntryPoint = "rust_engine_destroy")]
    internal static extern void Destroy(IntPtr engine);

    [DllImport(LibraryName, EntryPoint = "rust_engine_set_control_input")]
    internal static extern void SetControlInput(IntPtr engine, ControlInput input);

    [DllImport(LibraryName, EntryPoint = "rust_engine_tick")]
    internal static extern void Tick(IntPtr engine, float dtSeconds);

    [DllImport(LibraryName, EntryPoint = "rust_engine_render_state")]
    internal static extern EarthRenderState RenderState(IntPtr engine);

    [DllImport(LibraryName, EntryPoint = "rust_engine_surface_patches")]
    internal static extern SurfacePatchView SurfacePatches(IntPtr engine);

    private static IntPtr ResolveLibrary(
        string libraryName,
        Assembly assembly,
        DllImportSearchPath? searchPath)
    {
        if (libraryName != LibraryName)
        {
            return IntPtr.Zero;
        }

        string projectLibraryPath = GetProjectLibraryPath();
        if (File.Exists(projectLibraryPath))
        {
            return NativeLibrary.Load(projectLibraryPath);
        }

        return NativeLibrary.Load(libraryName, assembly, searchPath);
    }

    private static string GetProjectLibraryPath()
    {
        if (OperatingSystem.IsMacOS())
        {
            return ProjectSettings.GlobalizePath("res://native/macos/librust_engine.dylib");
        }

        if (OperatingSystem.IsLinux())
        {
            return ProjectSettings.GlobalizePath("res://native/linux/librust_engine.so");
        }

        if (OperatingSystem.IsWindows())
        {
            return ProjectSettings.GlobalizePath("res://native/windows/rust_engine.dll");
        }

        throw new PlatformNotSupportedException("No Rust engine native library path for this OS.");
    }
}

