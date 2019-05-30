/*
 * gpu.h
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#ifndef GPU_H_
#define GPU_H_

#include <cstdint>
#include <vector>

class MemoryController;

typedef std::vector<uint8_t> Tile;

class GPU {
public:
    GPU(MemoryController & memory);
    ~GPU() = default;

private:
    static const uint16_t TILE_SIZE;
    static const uint16_t TILES_PER_SET;

    static const uint16_t TILE_SET_0_OFFSET;
    static const uint16_t TILE_SET_1_OFFSET;

    static const uint16_t TILE_MAP_ROWS;
    static const uint16_t TILE_MAP_COLUMNS;

    static const uint16_t TILE_MAP_0_OFFSET;
    static const uint16_t TILE_MAP_1_OFFSET;

    static const uint16_t SCREEN_ROWS;
    static const uint16_t SCREEN_COLUMNS;

    enum MapIndex {
        MAP_0 = 0,
        MAP_1 = 1,
    };

    struct TilePtr {
        union { uint8_t tile0; int8_t tile1; };
    };

    MemoryController & m_memory;

    uint8_t m_pallete;
    uint8_t m_x;
    uint8_t m_y;

    Tile lookup(uint16_t address);
    Tile lookup(MapIndex index, uint16_t x, uint16_t y);
};

#endif /* SRC_GPU_H_ */
