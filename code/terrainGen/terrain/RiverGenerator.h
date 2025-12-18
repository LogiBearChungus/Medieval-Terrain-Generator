#pragma once
#include "world\World.h"
#include <vector>
#include <queue>

struct FlowCell {
    int x, y;
    float height;
    float waterAccumulation;

    bool operator<(const FlowCell& other) const {
        // Higher elevation = higher priority (for max heap)
        return height < other.height;
    }
};

class RiverGenerator {
public:
    RiverGenerator(World& world);

    // Generate rivers using precipitation and flow accumulation
    void generateRivers(
        int numSources = 50,           // Number of water sources to spawn
        float riverThreshold = 0.15f,  // Accumulation needed to form river
        float moistureInfluence = 0.5f // How much moisture affects water sources
    );

    // Generate lakes in low-lying areas
    void generateLakes(float lakeThreshold = 0.05f);

private:
    World& m_world;

    // Flow direction map: which neighbor does water flow to?
    std::vector<int> m_flowDirection; // -1 = ocean/sink, 0-7 = direction index

    // Water accumulation per tile
    std::vector<float> m_accumulation;

    // Calculate flow directions for all tiles
    void calculateFlowDirections();

    // Find the steepest downhill neighbor
    int findSteepestNeighbor(int x, int y) const;

    // Simulate water flow from sources
    void simulateFlow(int startX, int startY, float waterAmount);

    // Get flow direction offsets
    static const int DX[8];
    static const int DY[8];
};