using System;
using System.Runtime.InteropServices;
using UnityEngine;

namespace RustUnityPoc
{
    public sealed class RustSimulationRenderer : MonoBehaviour
    {
        private IntPtr engine;
        private Transform globeRoot;
        private Transform landRoot;
        private GameObject earthSphere;
        private GameObject atmosphereSphere;
        private Material oceanMaterial;
        private Material landMaterial;
        private Material atmosphereMaterial;
        private Light keyLight;
        private EngineEventCallback engineEventCallback;
        private GCHandle selfHandle;
        private EarthRenderState latestRustState;
        private bool hasLatestRustState;

        private void Awake()
        {
            oceanMaterial = BuildMaterial(new Color(0.05f, 0.26f, 0.65f));
            landMaterial = BuildMaterial(new Color(0.16f, 0.47f, 0.22f));
            atmosphereMaterial = BuildTransparentMaterial(new Color(0.45f, 0.72f, 1.0f, 0.24f));
        }

        private void OnEnable()
        {
            engine = RustEngineNative.Create();
            engineEventCallback = OnRustEngineEvent;
            selfHandle = GCHandle.Alloc(this);
            RustEngineNative.SetEventCallback(engine, engineEventCallback, GCHandle.ToIntPtr(selfHandle));
            BuildGlobe();
            BuildSurfacePatches();
        }

        private void OnDisable()
        {
            if (globeRoot != null)
            {
                Destroy(globeRoot.gameObject);
            }

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

        private void Update()
        {
            if (engine == IntPtr.Zero)
            {
                return;
            }

            RustEngineNative.SetControlInput(engine, ReadInput());
            RustEngineNative.Tick(engine, Time.deltaTime);
            ApplyRustState(hasLatestRustState ? latestRustState : RustEngineNative.RenderState(engine));
        }

        private void HandleRustEngineEvent(EngineEvent engineEvent)
        {
            if (engineEvent.kind != 1)
            {
                return;
            }

            latestRustState = engineEvent.state;
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

        private static ControlInput ReadInput()
        {
            float rotateX = 0.0f;
            float rotateY = 0.0f;
            float zoom = 0.0f;

            if (Input.GetKey(KeyCode.UpArrow))
            {
                rotateX += 1.0f;
            }

            if (Input.GetKey(KeyCode.DownArrow))
            {
                rotateX -= 1.0f;
            }

            if (Input.GetKey(KeyCode.LeftArrow))
            {
                rotateY += 1.0f;
            }

            if (Input.GetKey(KeyCode.RightArrow))
            {
                rotateY -= 1.0f;
            }

            if (Input.GetKey(KeyCode.Equals) || Input.GetKey(KeyCode.KeypadPlus) || Input.GetKey(KeyCode.PageUp))
            {
                zoom += 1.0f;
            }

            if (Input.GetKey(KeyCode.Minus) || Input.GetKey(KeyCode.KeypadMinus) || Input.GetKey(KeyCode.PageDown))
            {
                zoom -= 1.0f;
            }

            zoom += Mathf.Clamp(Input.mouseScrollDelta.y, -1.0f, 1.0f);

            return new ControlInput
            {
                rotateX = rotateX,
                rotateY = rotateY,
                zoom = zoom,
                reset = Input.GetKey(KeyCode.R) ? 1u : 0u,
            };
        }

        private void BuildGlobe()
        {
            GameObject root = new("Rust Earth");
            globeRoot = root.transform;

            earthSphere = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            earthSphere.name = "Earth Ocean";
            earthSphere.transform.SetParent(globeRoot, false);
            earthSphere.GetComponent<Renderer>().sharedMaterial = oceanMaterial;

            landRoot = new GameObject("Earth Land").transform;
            landRoot.SetParent(globeRoot, false);

            atmosphereSphere = GameObject.CreatePrimitive(PrimitiveType.Sphere);
            atmosphereSphere.name = "Atmosphere";
            atmosphereSphere.transform.SetParent(globeRoot, false);
            atmosphereSphere.GetComponent<Renderer>().sharedMaterial = atmosphereMaterial;
        }

        private unsafe void BuildSurfacePatches()
        {
            SurfacePatchView view = RustEngineNative.SurfacePatches(engine);
            if (view.ptr == IntPtr.Zero || view.len == UIntPtr.Zero)
            {
                return;
            }

            SurfacePatch* patches = (SurfacePatch*)view.ptr.ToPointer();
            ulong count = view.len.ToUInt64();
            for (ulong index = 0; index < count; index++)
            {
                SurfacePatch patch = patches[index];
                Vector3 normal = SphericalNormal(patch.latDegrees, patch.lonDegrees);
                GameObject land = GameObject.CreatePrimitive(PrimitiveType.Sphere);
                land.name = $"Land Patch {index}";
                land.transform.SetParent(landRoot, false);
                land.transform.localPosition = normal * 1.012f;
                land.transform.localRotation = Quaternion.FromToRotation(Vector3.forward, normal);
                float size = patch.radiusDegrees / 90.0f;
                land.transform.localScale = new Vector3(size * patch.stretchX, size * patch.stretchY, 0.018f);
                land.GetComponent<Renderer>().sharedMaterial = landMaterial;
            }
        }

        private void ApplyRustState(EarthRenderState state)
        {
            globeRoot.rotation = Quaternion.Euler(
                state.rotationX * Mathf.Rad2Deg,
                state.rotationY * Mathf.Rad2Deg,
                0.0f);

            float earthDiameter = state.radius * 2.0f;
            earthSphere.transform.localScale = Vector3.one * earthDiameter;
            atmosphereSphere.transform.localScale = Vector3.one * (state.atmosphereRadius * 2.0f);

            Camera camera = Camera.main;
            if (camera != null)
            {
                camera.transform.position = new Vector3(0.0f, 0.0f, -state.cameraDistance);
                camera.transform.LookAt(Vector3.zero, Vector3.up);
            }

            if (keyLight != null)
            {
                keyLight.transform.rotation = Quaternion.LookRotation(new Vector3(state.lightX, state.lightY, state.lightZ));
            }
        }

        internal void SetKeyLight(Light light)
        {
            keyLight = light;
        }

        private static Vector3 SphericalNormal(float latDegrees, float lonDegrees)
        {
            float lat = latDegrees * Mathf.Deg2Rad;
            float lon = lonDegrees * Mathf.Deg2Rad;
            float cosLat = Mathf.Cos(lat);

            return new Vector3(
                cosLat * Mathf.Sin(lon),
                Mathf.Sin(lat),
                cosLat * Mathf.Cos(lon)).normalized;
        }

        private static Material BuildMaterial(Color color)
        {
            Shader shader = Shader.Find("Universal Render Pipeline/Lit");
            if (shader == null)
            {
                shader = Shader.Find("Standard");
            }

            return new Material(shader)
            {
                color = color,
            };
        }

        private static Material BuildTransparentMaterial(Color color)
        {
            Material material = BuildMaterial(color);
            material.color = color;
            material.SetFloat("_Mode", 3.0f);
            material.SetInt("_SrcBlend", (int)UnityEngine.Rendering.BlendMode.SrcAlpha);
            material.SetInt("_DstBlend", (int)UnityEngine.Rendering.BlendMode.OneMinusSrcAlpha);
            material.SetInt("_ZWrite", 0);
            material.EnableKeyword("_ALPHABLEND_ON");
            material.renderQueue = 3000;
            return material;
        }
    }
}
