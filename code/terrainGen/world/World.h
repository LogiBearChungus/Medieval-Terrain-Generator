#pragma once

#include <vector>
#include "tile.h"

class World {
public:
    World(int width, int height);

    // Dimensions
    int getWidth() const;
    int getHeight() const;

    // Tile access (safe)
    Tile& at(int x, int y);
    const Tile& at(int x, int y) const;

    // Bounds check
    bool inBounds(int x, int y) const;

    // Utilities
    void clear();

private:
    int m_width;
    int m_height;

    std::vector<Tile> m_tiles;

    int index(int x, int y) const;
};
#pragma once
