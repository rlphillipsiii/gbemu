/*
 * gpu.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <cassert>

#include "gpu.h"
#include "processor.h"
#include "memorycontroller.h"
#include "memmap.h"
#include "logging.h"

/** 16 byte tile size (8x8 bit tile w/ 2 bytes per pixel) */
const uint16_t GPU::TILE_SIZE = 16;

const uint16_t GPU::TILES_PER_SET = 256;

/** Tile set 0 starts at the beginning of the GPU ram area of the memory controller */
const uint16_t GPU::TILE_SET_0_OFFSET = GPU_RAM_OFFSET;

/** Tile set 1 shares the second 128 tiles of tile set 0 */
const uint16_t GPU::TILE_SET_1_OFFSET = TILE_SET_0_OFFSET + ((TILES_PER_SET * TILE_SIZE) / 2);

/** Each tile map is 32x32 tiles, which equates to 256x256 pixels */
const uint16_t GPU::TILE_MAP_ROWS    = 32;
const uint16_t GPU::TILE_MAP_COLUMNS = 32;

/** Tile map 0 comes directly after the end of tile set 1 */
const uint16_t GPU::TILE_MAP_0_OFFSET = TILE_SET_1_OFFSET + (TILES_PER_SET * TILE_SIZE);

/** Tile map 1 come directly after the end of tile map 0 */
const uint16_t GPU::TILE_MAP_1_OFFSET = TILE_MAP_1_OFFSET + (TILE_MAP_ROWS * TILE_MAP_COLUMNS);

const uint16_t GPU::OAM_TICKS    = 80;
const uint16_t GPU::VRAM_TICKS   = 172;
const uint16_t GPU::HBLANK_TICKS = 204;
const uint16_t GPU::VBLANK_TICKS = 456;

const uint16_t GPU::ROW_COUNT = 144;
const uint16_t GPU::COL_COUNT = 160;

GPU::GPU(MemoryController & memory)
    : m_memory(memory),
      m_cpu(nullptr)
{
    reset();
}

void GPU::reset()
{
    m_state = OAM;
    
    m_x = m_y = 0;
    
    m_ticks = 0;
}

GPU::RenderState GPU::next()
{
    switch (m_state) {
    case HBLANK: {
        if (HBLANK_TICKS == m_ticks) {
            return (ROW_COUNT == m_scanline) ? VBLANK : OAM;
        }
        break;
    }
    case VBLANK: {
        if (VBLANK_TICKS == m_ticks) {
            return HBLANK;
        }
        break;
    }
    case OAM: {
        if (OAM_TICKS == m_ticks) {
            return VRAM;
        }
        break;
    }
    case VRAM: {
        if (VRAM_TICKS == m_ticks) {
            return HBLANK;
        }
        break;
    }
    default: {
        LOG("GPU::next() : Unknown render state %d\n", m_state);
    
        assert(0);
        break;
    }
    }
    
    return m_state;
}

void GPU::cycle()
{
    m_ticks++;

    switch (m_state) {
    case HBLANK: handleHBlank(); break;
    case VBLANK: handleVBlank(); break;
    case OAM:    handleOAM();    break;
    case VRAM:   handleVRAM();   break;
    default:
        LOG("GPU::cycle : Unknown GPU state %d\n", m_state);
        assert(0);
        return;
    }
    
    RenderState current = m_state;
    m_state = next();

    if (current != m_state) {
        m_ticks = 0;
    }
}

void GPU::handleHBlank()
{
    if (m_ticks < HBLANK_TICKS) {
        return;
    }

    m_scanline++;
    if (ROW_COUNT == m_scanline) {
        m_cpu->setVBlankInterrupt();
    }
}

void GPU::handleVBlank()
{
    if (m_ticks < VBLANK_TICKS) {
        return;
    }

    m_scanline = 0;
}

void GPU::handleOAM()
{

}

void GPU::handleVRAM()
{

}

Tile GPU::lookup(uint16_t address)
{
    Tile tile(TILE_SIZE);
    for (size_t i = 0; i < tile.size(); i++) {
        tile[i] = m_memory.read(address + i);
    }

    return tile;
}

Tile GPU::lookup(MapIndex index, uint16_t x, uint16_t y)
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
