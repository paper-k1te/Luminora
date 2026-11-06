using System;
using UnityEngine;

namespace Rendering
{
    /// <summary>
    /// Master celestial lighting and time-of-day simulation for 2.5D sandbox worlds.
    /// Manages day/night progression, dynamic directional sun/moon rotation,
    /// ambient illumination for 3D bevel shading, and camera background clear colors.
    /// </summary>
    [ExecuteAlways]
    public class DayNightCycle : MonoBehaviour
    {
        public static DayNightCycle Instance { get; private set; }

        [Header("Time Simulation")]
        [Tooltip("Length of a full in-game day in real-time minutes.")]
        [SerializeField] private float dayDurationMinutes = 24.0f;

        [Tooltip("Current normalized time of day [0.0 = 06:00 Dawn, 0.25 = 12:00 Noon, 0.5 = 18:00 Sunset, 0.75 = 00:00 Midnight].")]
        [Range(0f, 1f)]
        [SerializeField] private float timeOfDay = 0.25f;

        [SerializeField] private bool autoAdvanceTime = true;

        [Header("Connected Components")]
        [SerializeField] private Light directionalLight;
        [SerializeField] private Camera targetCamera;

        [Header("Celestial Gradients")]
        [SerializeField] private Gradient skyColorGradient;
        [SerializeField] private Gradient lightColorGradient;
        [SerializeField] private Gradient ambientColorGradient;
        [SerializeField] private AnimationCurve lightIntensityCurve;

        // Public properties
        public float TimeOfDay => timeOfDay;
        public float DayDurationMinutes
        {
            get => dayDurationMinutes;
            set => dayDurationMinutes = Mathf.Max(0.1f, value);
        }
        public bool AutoAdvanceTime
        {
            get => autoAdvanceTime;
            set => autoAdvanceTime = value;
        }

        public bool IsNight => timeOfDay > 0.55f && timeOfDay < 0.95f;
        public Color CurrentSkyColor { get; private set; }
        public Color CurrentLightColor { get; private set; }
        public Color CurrentAmbientColor { get; private set; }
        public float CurrentLightIntensity { get; private set; }

        public event Action<float> OnTimeChanged;

        private void Awake()
        {
            if (Instance == null) Instance = this;
            EnsureDefaults();
        }

        private void OnEnable()
        {
            if (Instance == null) Instance = this;
            EnsureDefaults();
        }

        private void Start()
        {
            if (targetCamera == null) targetCamera = Camera.main;
            if (directionalLight == null) directionalLight = FindDirectionalLight();
            UpdateLighting(timeOfDay);
        }

        private void Update()
        {
            if (Application.isPlaying && autoAdvanceTime && dayDurationMinutes > 0f)
            {
                float timePerSecond = 1.0f / (dayDurationMinutes * 60.0f);
                timeOfDay = (timeOfDay + timePerSecond * Time.deltaTime) % 1.0f;
                OnTimeChanged?.Invoke(timeOfDay);
            }

            UpdateLighting(timeOfDay);
        }

        /// <summary>
        /// Explicitly modifies the time of day.
        /// </summary>
        public void SetTimeOfDay(float normalizedTime)
        {
            timeOfDay = Mathf.Repeat(normalizedTime, 1.0f);
            UpdateLighting(timeOfDay);
            OnTimeChanged?.Invoke(timeOfDay);
        }

        /// <summary>
        /// Updates skybox/camera, directional sunlight/moonlight, and ambient colors.
        /// </summary>
        public void UpdateLighting(float time)
        {
            if (skyColorGradient == null || lightColorGradient == null)
            {
                EnsureDefaults();
            }

            // Sample curves
            CurrentSkyColor = skyColorGradient.Evaluate(time);
            CurrentLightColor = lightColorGradient.Evaluate(time);
            CurrentAmbientColor = ambientColorGradient.Evaluate(time);
            CurrentLightIntensity = lightIntensityCurve != null ? lightIntensityCurve.Evaluate(time) : 1.0f;

            // Apply camera clear color
            if (targetCamera == null) targetCamera = Camera.main;
            if (targetCamera != null)
            {
                targetCamera.clearFlags = CameraClearFlags.SolidColor;
                targetCamera.backgroundColor = CurrentSkyColor;
            }

            // Apply directional sun/moon light
            if (directionalLight == null) directionalLight = FindDirectionalLight();
            if (directionalLight != null)
            {
                directionalLight.color = CurrentLightColor;
                directionalLight.intensity = CurrentLightIntensity;

                // Orbit sun/moon in 2.5D space with slight pitch tilt to cast dramatic bevel highlights
                // 0.0 = Sun rising in East (Left, -X)
                // 0.25 = High Noon (+Y)
                // 0.5 = Sunset in West (Right, +X)
                // 0.75 = Midnight Moon
                float sunAngle = (time * 360.0f) - 90.0f;
                directionalLight.transform.rotation = Quaternion.Euler(35.0f, sunAngle, 0.0f);
            }

            // Update ambient lighting for 2.5D side-bevel pop
            RenderSettings.ambientMode = UnityEngine.Rendering.AmbientMode.Flat;
            RenderSettings.ambientLight = CurrentAmbientColor;
        }

        private Light FindDirectionalLight()
        {
            Light[] lights = FindObjectsOfType<Light>();
            for (int i = 0; i < lights.Length; i++)
            {
                if (lights[i].type == LightType.Directional)
                {
                    return lights[i];
                }
            }

            // Create one if none found
            GameObject lightObj = new GameObject("SunLight_Directional");
            Light l = lightObj.AddComponent<Light>();
            l.type = LightType.Directional;
            l.shadows = LightShadows.Soft;
            return l;
        }

        /// <summary>
        /// Populates default publication-grade atmospheric gradients and curves.
        /// </summary>
        public void EnsureDefaults()
        {
            if (skyColorGradient == null || skyColorGradient.colorKeys.Length <= 1)
            {
                skyColorGradient = new Gradient();
                skyColorGradient.SetKeys(
                    new GradientColorKey[]
                    {
                        new GradientColorKey(new Color(1.0f, 0.55f, 0.35f), 0.00f), // Dawn orange/pink
                        new GradientColorKey(new Color(0.40f, 0.72f, 0.98f), 0.25f), // Noon brilliant cyan-blue
                        new GradientColorKey(new Color(0.70f, 0.28f, 0.45f), 0.50f), // Sunset crimson-violet
                        new GradientColorKey(new Color(0.04f, 0.06f, 0.12f), 0.75f), // Midnight deep space navy
                        new GradientColorKey(new Color(1.0f, 0.55f, 0.35f), 1.00f)  // Dawn loop
                    },
                    new GradientAlphaKey[]
                    {
                        new GradientAlphaKey(1.0f, 0.0f),
                        new GradientAlphaKey(1.0f, 1.0f)
                    }
                );
            }

            if (lightColorGradient == null || lightColorGradient.colorKeys.Length <= 1)
            {
                lightColorGradient = new Gradient();
                lightColorGradient.SetKeys(
                    new GradientColorKey[]
                    {
                        new GradientColorKey(new Color(1.00f, 0.78f, 0.50f), 0.00f), // Dawn gold
                        new GradientColorKey(new Color(1.00f, 0.98f, 0.92f), 0.25f), // Noon white
                        new GradientColorKey(new Color(1.00f, 0.45f, 0.25f), 0.50f), // Sunset flame
                        new GradientColorKey(new Color(0.35f, 0.48f, 0.75f), 0.75f), // Midnight pale moon
                        new GradientColorKey(new Color(1.00f, 0.78f, 0.50f), 1.00f)
                    },
                    new GradientAlphaKey[]
                    {
                        new GradientAlphaKey(1.0f, 0.0f),
                        new GradientAlphaKey(1.0f, 1.0f)
                    }
                );
            }

            if (ambientColorGradient == null || ambientColorGradient.colorKeys.Length <= 1)
            {
                ambientColorGradient = new Gradient();
                ambientColorGradient.SetKeys(
                    new GradientColorKey[]
                    {
                        new GradientColorKey(new Color(0.45f, 0.35f, 0.30f), 0.00f),
                        new GradientColorKey(new Color(0.55f, 0.58f, 0.62f), 0.25f),
                        new GradientColorKey(new Color(0.38f, 0.25f, 0.32f), 0.50f),
                        new GradientColorKey(new Color(0.12f, 0.14f, 0.22f), 0.75f),
                        new GradientColorKey(new Color(0.45f, 0.35f, 0.30f), 1.00f)
                    },
                    new GradientAlphaKey[]
                    {
                        new GradientAlphaKey(1.0f, 0.0f),
                        new GradientAlphaKey(1.0f, 1.0f)
                    }
                );
            }

            if (lightIntensityCurve == null || lightIntensityCurve.length <= 1)
            {
                lightIntensityCurve = new AnimationCurve();
                lightIntensityCurve.AddKey(0.00f, 0.85f); // Dawn
                lightIntensityCurve.AddKey(0.25f, 1.15f); // Noon
                lightIntensityCurve.AddKey(0.50f, 0.75f); // Sunset
                lightIntensityCurve.AddKey(0.60f, 0.25f); // Night fall
                lightIntensityCurve.AddKey(0.75f, 0.20f); // Midnight
                lightIntensityCurve.AddKey(0.90f, 0.25f); // Pre-dawn
                lightIntensityCurve.AddKey(1.00f, 0.85f); // Dawn loop
            }
        }
    }
}
