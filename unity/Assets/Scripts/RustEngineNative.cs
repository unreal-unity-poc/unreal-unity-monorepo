using System;
using System.Runtime.InteropServices;

namespace RustUnityPoc
{
    [StructLayout(LayoutKind.Sequential)]
    public struct ControlInput
    {
        public float rotateX;
        public float rotateY;
        public float zoom;
        public uint reset;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct EarthRenderState
    {
        public float radius;
        public float atmosphereRadius;
        public float rotationX;
        public float rotationY;
        public float cloudRotationY;
        public float cameraDistance;
        public float lightX;
        public float lightY;
        public float lightZ;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SurfacePatch
    {
        public float latDegrees;
        public float lonDegrees;
        public float radiusDegrees;
        public float stretchX;
        public float stretchY;
    }

    [StructLayout(LayoutKind.Sequential)]
    public struct SurfacePatchView
    {
        public IntPtr ptr;
        public UIntPtr len;
    }

    internal static class RustEngineNative
    {
        private const string LibraryName = "rust_engine";

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
    }
}

