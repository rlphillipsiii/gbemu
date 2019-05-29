/*
 * gpu.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include "gpu.h"
#include "memorycontroller.h"
#include "memmap.h"

/** 16 byte tile size (8x8 bit tile w/ 2 bytes per pixel) */
const uint16_t Gpu::TILE_SIZE = 16;

const uint16_t Gpu::TILES_PER_SET = 256;

/** Tile set 0 starts at the beginning of the GPU ram area of the memory controller */
const uint16_t Gpu::TILE_SET_0_OFFSET = GPU_RAM_OFFSET;

/** Tile set 1 shares the second 128 tiles of tile set 0 */
const uint16_t Gpu::TILE_SET_1_OFFSET = TILE_SET_0_OFFSET + ((TILES_PER_SET * TILE_SIZE) / 2);

/** Each tile map is 32x32 tiles, which equates to 256x256 pixels */
const uint16_t Gpu::TILE_MAP_ROWS    = 32;
const uint16_t Gpu::TILE_MAP_COLUMNS = 32;

/** Tile map 0 comes directly after the end of tile set 1 */
const uint16_t Gpu::TILE_MAP_0_OFFSET = TILE_SET_1_OFFSET + (TILES_PER_SET * TILE_SIZE);

/** Tile map 1 come directly after the end of tile map 0 */
const uint16_t Gpu::TILE_MAP_1_OFFSET = TILE_MAP_1_OFFSET + (TILE_MAP_ROWS * TILE_MAP_COLUMNS);

Gpu::Gpu(MemoryController & memory)
    : m_memory(memory)
{

}

Tile Gpu::lookup(uint16_t address)
{
    Tile tile(TILE_SIZE);
    for (size_t i = 0; i < tile.size(); i++) {
        tile[i] = m_memory.read(address + i);
    }

    return tile;
}

Tile Gpu::lookup(MapIndex index, uint16_t x, uint16_t y)
{
    // We have two maps, so figure out which offset we need to use for our address
    uint16_t offset = (MAP_0 == index) ? TILE_MAP_0_OFFSET : TILE_MAP_1_OFFSET;

    // Now that we know our offset, figure out the address of the tile that we are
    // needing to look up.
    uint16_t address = offset + ((x * TILE_MAP_COLUMNS) + y);

    // We have the address, so let's read out our actual value and shove it in to a
    // tile pointer object.  This will let us interpret the number that comes out as
    // either a signed integer (in the case of map 1) or an unsigned integer (in the
    // case of map 2).
    TilePtr tptr;
    tptr.tile0 = m_memory.read(address);

    // Actually figure out the location of the tile that we want.  If we are after a
    // tile in the second set, then we need to set our offset to the end of the 1st
    // set and use our signed number to index in to the second set because the 2nd
    // half of the 1st set is also the 1st half the second set.
    uint16_t location = (MAP_0 == index) ?
            TILE_SET_0_OFFSET + tptr.tile0 : TILE_SET_0_OFFSET + tptr.tile1 + (TILES_PER_SET * TILE_SIZE);

    return lookup(location);
}
