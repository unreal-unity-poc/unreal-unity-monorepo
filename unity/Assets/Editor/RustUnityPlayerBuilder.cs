using System.IO;
using UnityEditor;
using UnityEditor.Build;
using UnityEditor.SceneManagement;
using UnityEngine;
using UnityEngine.SceneManagement;

namespace RustUnityPoc.Editor
{
    public static class RustUnityPlayerBuilder
    {
        private const string ScenePath = "Assets/Scenes/RustEarth.unity";
        private const string BuildPath = "Builds/RustUnityRenderer.app";

        public static void BuildMacPlayer()
        {
            EnsureRuntimeScene();

            PlayerSettings.productName = "Rust Unity Renderer";
            PlayerSettings.companyName = "Rust Unity Unreal POC";
            PlayerSettings.SetApplicationIdentifier(
                NamedBuildTarget.Standalone,
                "dev.rustunityunrealpoc.rustunityrenderer");

            BuildPipeline.BuildPlayer(
                new[] { ScenePath },
                BuildPath,
                BuildTarget.StandaloneOSX,
                BuildOptions.CleanBuildCache);
        }

        private static void EnsureRuntimeScene()
        {
            Directory.CreateDirectory("Assets/Scenes");

            if (File.Exists(ScenePath))
            {
                return;
            }

            Scene scene = EditorSceneManager.NewScene(NewSceneSetup.EmptyScene, NewSceneMode.Single);
            scene.name = "RustEarth";
            EditorSceneManager.SaveScene(scene, ScenePath);
            AssetDatabase.Refresh();
        }
    }
}
