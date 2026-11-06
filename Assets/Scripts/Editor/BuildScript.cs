#if UNITY_EDITOR
using System;
using System.IO;
using UnityEditor;
using UnityEditor.Build.Reporting;
using UnityEngine;

namespace Editor
{
    /// <summary>
    /// Automated batch and menu build script for exporting the complete 2.5D Luminora sandbox
    /// as a standalone Windows 64-bit executable (.exe).
    /// </summary>
    public static class BuildScript
    {
        private const string ScenePath = "Assets/Scenes/SandboxWorld.unity";
        private const string OutputDirectory = "Build/Windows";
        private const string ExeName = "Luminora_Sandbox.exe";

        [MenuItem("Sandbox 2.5D/Build Windows Executable (.exe)", false, 10)]
        public static void BuildWindowsExecutableMenu()
        {
            BuildWindowsPlayer();
        }

        public static void BuildWindowsPlayer()
        {
            Debug.Log("[BuildScript] Starting Windows Standalone x64 Build...");

            string projectRoot = Path.GetDirectoryName(Application.dataPath);
            string buildFolder = Path.Combine(projectRoot, OutputDirectory);
            if (!Directory.Exists(buildFolder))
            {
                Directory.CreateDirectory(buildFolder);
            }

            string exeFullPath = Path.Combine(buildFolder, ExeName);

            // Configure build options
            BuildPlayerOptions buildPlayerOptions = new BuildPlayerOptions
            {
                scenes = new[] { ScenePath },
                locationPathName = exeFullPath,
                target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.None
            };

            BuildReport report = BuildPipeline.BuildPlayer(buildPlayerOptions);
            BuildSummary summary = report.summary;

            if (summary.result == BuildResult.Succeeded)
            {
                Debug.Log($"[BuildScript] Build SUCCESSFUL: {summary.totalSize / (1024 * 1024)} MB");
                Debug.Log($"[BuildScript] Output executable: {exeFullPath}");
                EditorUtility.RevealInFinder(exeFullPath);
            }
            else if (summary.result == BuildResult.Failed)
            {
                Debug.LogError($"[BuildScript] Build FAILED! Total errors: {summary.totalErrors}");
            }
        }
    }
}
#endif
