#include "RiverGenerator.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <ctime>

// 8-directional neighbors (N, NE, E, SE, S, SW, W, NW)
const int RiverGenerator::DX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
const int RiverGenerator::DY[8] = { -1, -1, 0, 1, 1, 1, 0, -1 };

RiverGenerator::RiverGenerator(World& world)
    : m_world(world),
    m_flowDirection(world.getWidth()* world.getHeight(), -1),
    m_accumulation(world.getWidth()* world.getHeight(), 0.0f)
{
}

void RiverGenerator::generateRivers(int numSources, float riverThreshold, float moistureInfluence) {
    int width = m_world.getWidth();
    int height = m_world.getHeight();

    // Step 1: Calculate flow directions for all tiles
    calculateFlowDirections();

    // Step 2: Spawn water sources (prefer high elevation + high moisture)
    std::vector<std::pair<int, int>> sources;
    std::mt19937 rng(static_cast<unsigned>(std::time(nullptr)));

    // Collect valid source candidates (land tiles above sea level)
    std::vector<std::pair<int, int>> candidates;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const Tile& tile = m_world.at(x, y);

            // Must be land and not too low
            if (tile.height > 0.47f && tile.biome != Biome::Ocean && tile.biome != Biome::Beach) {
                // Weight by height and moisture
                float weight = tile.height * (1.0f - moistureInfluence) +
                    tile.moisture * moistureInfluence;

                // Higher elevation and wetter areas are better sources
                if (weight > 0.5f) {
                    candidates.push_back({ x, y });
                }
            }
        }
    }

    // Randomly select sources from candidates
    std::shuffle(candidates.begin(), candidates.end(), rng);
    int actualSources = std::min(numSources, (int)candidates.size());

    for (int i = 0; i < actualSources; ++i) {
        sources.push_back(candidates[i]);
    }

    // Step 3: Simulate water flow from each source
    for (const auto& source : sources) {
        // More water from wetter/higher areas
        float waterAmount = 0.02f + m_world.at(source.first, source.second).moisture * 0.03f;
        simulateFlow(source.first, source.second, waterAmount);
    }

    // Step 4: Convert accumulation to river strength
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            Tile& tile = m_world.at(x, y);

            // Only create rivers on land
            if (tile.biome != Biome::Ocean && tile.biome != Biome::Beach) {
                if (m_accumulation[idx] > riverThreshold) {
                    // Normalize river strength (stronger rivers have more accumulation)
                    tile.riverStrength = std::min(1.0f, m_accumulation[idx] / (riverThreshold * 5.0f));
                }
            }
        }
    }
}

void RiverGenerator::calculateFlowDirections() {
    int width = m_world.getWidth();
    int height = m_world.getHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            const Tile& tile = m_world.at(x, y);

            // Ocean tiles are sinks
            if (tile.biome == Biome::Ocean) {
                m_flowDirection[idx] = -1;
                continue;
            }

            // Find steepest downhill neighbor
            m_flowDirection[idx] = findSteepestNeighbor(x, y);
        }
    }
}

int RiverGenerator::findSteepestNeighbor(int x, int y) const {
    int width = m_world.getWidth();
    int height = m_world.getHeight();

    const Tile& current = m_world.at(x, y);
    float currentHeight = current.height;

    int steepestDir = -1;
    float steepestSlope = 0.0f;

    // Check all 8 neighbors
    for (int dir = 0; dir < 8; ++dir) {
        int nx = x + DX[dir];
        int ny = y + DY[dir];

        if (!m_world.inBounds(nx, ny)) continue;

        const Tile& neighbor = m_world.at(nx, ny);
        float slope = currentHeight - neighbor.height;

        // Account for diagonal distance
        if (dir % 2 == 1) { // Diagonal
            slope *= 0.707f; // Adjust for sqrt(2) distance
        }

        // Water flows to steepest downhill neighbor (or ocean)
        if (slope > steepestSlope || neighbor.biome == Biome::Ocean) {
            steepestSlope = slope;
            steepestDir = dir;
        }
    }

    return steepestDir;
}

void RiverGenerator::simulateFlow(int startX, int startY, float waterAmount) {
    int width = m_world.getWidth();
    int x = startX;
    int y = startY;

    // Follow flow direction until we hit ocean or a sink
    int maxSteps = width * 2; // Prevent infinite loops
    int steps = 0;

    while (steps < maxSteps) {
        if (!m_world.inBounds(x, y)) break;

        int idx = y * width + x;
        const Tile& tile = m_world.at(x, y);

        // Add water to this cell
        m_accumulation[idx] += waterAmount;

        // If we hit ocean, stop
        if (tile.biome == Biome::Ocean) {
            break;
        }

        // Get flow direction
        int dir = m_flowDirection[idx];
        if (dir == -1) break; // No flow (local minimum or ocean)

        // Move to next cell
        x += DX[dir];
        y += DY[dir];
        steps++;

        // Water accumulates as it flows downstream
        waterAmount += 0.001f; // Slight increase (simulates tributaries/rain)
    }
}

void RiverGenerator::generateLakes(float lakeThreshold) {
    int width = m_world.getWidth();
    int height = m_world.getHeight();

    // Lakes form in local minima with enough water accumulation
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int idx = y * width + x;
            Tile& tile = m_world.at(x, y);

            // Must be land, not already ocean/beach
            if (tile.biome == Biome::Ocean || tile.biome == Biome::Beach) {
                continue;
            }

            // Check if this is a local minimum with water
            if (m_flowDirection[idx] == -1 && m_accumulation[idx] > lakeThreshold) {
                tile.isLake = true;

                // Optionally flood nearby low areas
                for (int dir = 0; dir < 8; ++dir) {
                    int nx = x + DX[dir];
                    int ny = y + DY[dir];

                    if (m_world.inBounds(nx, ny)) {
                        Tile& neighbor = m_world.at(nx, ny);
                        if (neighbor.height <= tile.height + 0.02f &&
                            neighbor.biome != Biome::Ocean) {
                            neighbor.isLake = true;
                        }
                    }
                }
            }
        }
    }
}