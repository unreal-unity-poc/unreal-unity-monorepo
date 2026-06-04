using UnityEngine;

namespace RustUnityPoc
{
    public static class RustSimulationBootstrap
    {
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.BeforeSceneLoad)]
        private static void Start()
        {
            GameObject simulation = new("Rust Simulation");
            RustSimulationRenderer renderer = simulation.AddComponent<RustSimulationRenderer>();
            Object.DontDestroyOnLoad(simulation);

            EnsureCamera();
            renderer.SetKeyLight(EnsureLight());
        }

        private static void EnsureCamera()
        {
            if (Camera.main != null)
            {
                return;
            }

            GameObject cameraObject = new("Main Camera");
            Camera camera = cameraObject.AddComponent<Camera>();
            camera.tag = "MainCamera";
            camera.transform.position = new Vector3(0.0f, 0.0f, -4.2f);
            camera.transform.LookAt(Vector3.zero, Vector3.up);
            camera.clearFlags = CameraClearFlags.SolidColor;
            camera.backgroundColor = new Color(0.05f, 0.06f, 0.075f);
        }

        private static Light EnsureLight()
        {
            Light existing = Object.FindFirstObjectByType<Light>();
            if (existing != null)
            {
                return existing;
            }

            GameObject lightObject = new("Key Light");
            Light light = lightObject.AddComponent<Light>();
            light.type = LightType.Directional;
            light.intensity = 1.25f;
            light.transform.rotation = Quaternion.Euler(45.0f, -35.0f, 0.0f);
            return light;
        }
    }
}
