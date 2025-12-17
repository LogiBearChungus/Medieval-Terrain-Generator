#include "PerlinNoise.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <numeric>

// Constructors

PerlinNoise::PerlinNoise() {
    // Default seed
    PerlinNoise(1);
}

PerlinNoise::PerlinNoise(unsigned int seed) {
    p.resize(256);

    // Fill with values 0..255
    std::iota(p.begin(), p.end(), 0);

    // Shuffle with seed
    std::default_random_engine engine(seed);
    std::shuffle(p.begin(), p.end(), engine);

    // Duplicate the table
    p.insert(p.end(), p.begin(), p.end());
}

// Core Noise

float PerlinNoise::noise(float x, float y) const {
    // Find unit grid cell containing point
    int X = (int)std::floor(x) & 255;
    int Y = (int)std::floor(y) & 255;

    // Relative x,y in cell
    x -= std::floor(x);
    y -= std::floor(y);

    // Compute fade curves
    float u = fade(x);
    float v = fade(y);

    // Hash coordinates of corners
    int aa = p[p[X] + Y];
    int ab = p[p[X] + Y + 1];
    int ba = p[p[X + 1] + Y];
    int bb = p[p[X + 1] + Y + 1];

    // Add blended results
    float res = lerp(
        lerp(grad(aa, x, y), grad(ba, x - 1, y), u),
        lerp(grad(ab, x, y - 1), grad(bb, x - 1, y - 1), u),
        v
    );

    return res;
}

// Fractal / Octave Noise

float PerlinNoise::fractalNoise(float x, float y, int octaves, float lacunarity, float persistence) const {
    float total = 0.0f;
    float frequency = 1.0f;
    float amplitude = 1.0f;
    float maxAmplitude = 0.0f;

    for (int i = 1; i < octaves; i++) {
        total += noise(x * frequency, y * frequency) * amplitude;
        maxAmplitude += amplitude;

        amplitude *= persistence;
        frequency *= lacunarity;
    }

    // Normalize to [-1,1]
    return total / maxAmplitude;
}


// Helper Functions


float PerlinNoise::fade(float t) {
    // 6t^5 - 15t^4 + 10t^3
    return t * t * t * (t * (t * 6 - 15) + 10);
}

float PerlinNoise::lerp(float a, float b, float t) {
    return a + t * (b - a);
}

float PerlinNoise::grad(int hash, float x, float y) {
    int h = hash & 7;      // Convert low 3 bits of hash code
    float u = h < 4 ? x : y;
    float v = h < 4 ? y : x;

    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0f * v : 2.0f * v);
}
