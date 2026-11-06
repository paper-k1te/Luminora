using UnityEngine;

namespace Generation
{
    /// <summary>
    /// High-performance multi-octave coherent noise generator supporting Fractal Brownian Motion (fBm),
    /// domain warping, ridged multifractal sampling, and worm-cave tunnels.
    /// </summary>
    public static class NoiseGenerator
    {
        // Prime offset factors for decorrelating distinct noise passes
        private const float PrimeOffset1 = 173.31f;
        private const float PrimeOffset2 = 563.83f;
        private const float PrimeOffset3 = 827.47f;
        private const float PrimeOffset4 = 349.19f;

        /// <summary>
        /// Generates 1D multi-octave coherent noise normalized to [0, 1].
        /// </summary>
        public static float Fbm1D(
            float x,
            int seed,
            int octaves = 4,
            float frequency = 0.01f,
            float persistence = 0.5f,
            float lacunarity = 2.0f)
        {
            float total = 0f;
            float amplitude = 1f;
            float maxAmplitude = 0f;
            float currentFreq = frequency;

            float seedOffsetX = (seed * 19.37f) % 10000f;

            for (int i = 0; i < octaves; i++)
            {
                float sampleX = (x + seedOffsetX) * currentFreq;
                // Unity's PerlinNoise is 2D; evaluate along y = seedOffsetX for 1D slice
                float n = Mathf.PerlinNoise(sampleX, seedOffsetX + (i * 31.7f));
                total += n * amplitude;
                maxAmplitude += amplitude;

                amplitude *= persistence;
                currentFreq *= lacunarity;
            }

            return maxAmplitude > 0f ? total / maxAmplitude : 0.5f;
        }

        /// <summary>
        /// Generates 2D Fractal Brownian Motion (fBm) noise normalized to [0, 1].
        /// </summary>
        public static float Fbm2D(
            float x,
            float y,
            int seed,
            int octaves = 4,
            float frequency = 0.02f,
            float persistence = 0.5f,
            float lacunarity = 2.0f)
        {
            float total = 0f;
            float amplitude = 1f;
            float maxAmplitude = 0f;
            float currentFreq = frequency;

            float seedOffsetX = (seed * 17.13f) % 5000f;
            float seedOffsetY = (seed * 23.71f) % 5000f;

            for (int i = 0; i < octaves; i++)
            {
                float sampleX = (x + seedOffsetX) * currentFreq + (i * PrimeOffset1);
                float sampleY = (y + seedOffsetY) * currentFreq + (i * PrimeOffset2);

                float n = Mathf.PerlinNoise(sampleX, sampleY);
                total += n * amplitude;
                maxAmplitude += amplitude;

                amplitude *= persistence;
                currentFreq *= lacunarity;
            }

            return maxAmplitude > 0f ? total / maxAmplitude : 0.5f;
        }

        /// <summary>
        /// 2D Ridged multifractal noise useful for sharp peaks, crevices, and canyon features.
        /// Returns value in [0, 1], with 1.0 being sharpest ridges.
        /// </summary>
        public static float RidgedNoise2D(
            float x,
            float y,
            int seed,
            int octaves = 3,
            float frequency = 0.02f,
            float persistence = 0.5f,
            float lacunarity = 2.0f)
        {
            float total = 0f;
            float amplitude = 1f;
            float maxAmplitude = 0f;
            float currentFreq = frequency;

            float seedOffsetX = (seed * 13.11f) % 5000f;
            float seedOffsetY = (seed * 37.93f) % 5000f;

            for (int i = 0; i < octaves; i++)
            {
                float sampleX = (x + seedOffsetX) * currentFreq + (i * PrimeOffset3);
                float sampleY = (y + seedOffsetY) * currentFreq + (i * PrimeOffset4);

                // Invert the distance to median to form sharp crests
                float n = Mathf.PerlinNoise(sampleX, sampleY);
                float ridge = 1.0f - Mathf.Abs((n * 2.0f) - 1.0f);
                ridge *= ridge; // Square for sharper peak profile

                total += ridge * amplitude;
                maxAmplitude += amplitude;

                amplitude *= persistence;
                currentFreq *= lacunarity;
            }

            return maxAmplitude > 0f ? total / maxAmplitude : 0.5f;
        }

        /// <summary>
        /// Domain warping: perturbs input coordinates with secondary fBm vectors before sampling.
        /// Produces organic, non-repetitive fluid geological strata and natural subterranean caverns.
        /// </summary>
        public static float DomainWarpedNoise2D(
            float x,
            float y,
            int seed,
            float warpMagnitude = 18f,
            int octaves = 3,
            float frequency = 0.015f)
        {
            // Sample vector field (qx, qy) for coordinate deflection
            float qx = Fbm2D(x, y, seed + 101, octaves, frequency, 0.5f, 2.0f) - 0.5f;
            float qy = Fbm2D(x + 52.3f, y + 13.7f, seed + 202, octaves, frequency, 0.5f, 2.0f) - 0.5f;

            float warpedX = x + qx * warpMagnitude;
            float warpedY = y + qy * warpMagnitude;

            return Fbm2D(warpedX, warpedY, seed, octaves, frequency, 0.5f, 2.0f);
        }

        /// <summary>
        /// Tubular worm tunnel noise: produces continuous meandering subterranean channels.
        /// Returns true if the coordinate falls within a hollow tunnel path.
        /// </summary>
        public static bool IsWormTunnel(
            float x,
            float y,
            int seed,
            float frequency = 0.024f,
            float tunnelThickness = 0.08f)
        {
            // Two orthogonal sine/perlin tracks intersecting to carve ribbon-like worms
            float trackA = Fbm2D(x, y, seed + 505, 2, frequency, 0.5f, 2.0f);
            float trackB = Fbm2D(x + 120.7f, y + 73.1f, seed + 606, 2, frequency, 0.5f, 2.0f);

            // Channel exists when both tracks are close to 0.5 (zero crossing)
            float distA = Mathf.Abs(trackA - 0.5f);
            float distB = Mathf.Abs(trackB - 0.5f);

            return distA < tunnelThickness && distB < tunnelThickness * 1.5f;
        }
    }
}
