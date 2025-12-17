#pragma once

#include <vector>
#include <random>
#include <numeric>
#include <algorithm>

class PerlinNoise {
public:
    // Constructors
    PerlinNoise();
    PerlinNoise(unsigned int seed);

    // Core noise function
    float noise(float x, float y) const;

    // Fractal noise (multiple octaves)
    float fractalNoise(
        float x,
        float y,
        int octaves,
        float lacunarity,
        float persistence
    ) const;

private:
    std::vector<int> p; // permutation table

    // Helper functions
    static float fade(float t);
    static float lerp(float a, float b, float t);
    static float grad(int hash, float x, float y);
};
