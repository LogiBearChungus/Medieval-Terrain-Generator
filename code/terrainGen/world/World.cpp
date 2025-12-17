#include "world.h"
#include "tile.h"

#include <cassert>
#include <algorithm>

World::World(int width, int height)
    : m_width(width), m_height(height), m_tiles(width* height)
{
}

int World::getWidth() const {
    return m_width;
}

int World::getHeight() const {
    return m_height;
}

bool World::inBounds(int x, int y) const {
    return x >= 0 && x < m_width && y >= 0 && y < m_height;
}

int World::index(int x, int y) const {
    return y * m_width + x;
}

Tile& World::at(int x, int y) {
    assert(inBounds(x, y) && "World::at() out of bounds");
    return m_tiles[index(x, y)];
}

const Tile& World::at(int x, int y) const {
    assert(inBounds(x, y) && "World::at() out of bounds");
    return m_tiles[index(x, y)];
}

void World::clear() {
    std::fill(m_tiles.begin(), m_tiles.end(), Tile{});
}
