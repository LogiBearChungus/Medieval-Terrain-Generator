#pragma once

enum class Biome {
    Ocean,
    Beach,
    Plains,
    Forest,
    Desert,
    Tundra,
    Mountain,
    Swamp
};

enum class BiomeType {
    DeepOcean,
    Ocean,
    Beach,
    Grassland,
    Forest,
    DenseForest,
    Desert,
    Tundra,
    Taiga,
    Mountain,
    SnowPeak,
    Swamp,
    Savanna,
    Scrubland
};

struct Tile {
    // Core terrain
    float height = 0.0f;        // 0–1
    float moisture = 0.0f;      // 0–1
    float temperature = 0.0f;   // 0–1

    // Derived data
    Biome biome = Biome::Ocean;

    // Hydrology
    float riverStrength = 0.0f;
    bool isLake = false;

    // Infrastructure
    bool hasRoad = false;
    int settlementId = -1;
};
