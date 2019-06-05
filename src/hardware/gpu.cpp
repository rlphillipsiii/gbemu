/*
 * gpu.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <cassert>
#include <vector>

#include "gpu.h"
#include "processor.h"
#include "memorycontroller.h"
#include "memmap.h"
#include "logging.h"

using std::vector;

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

const uint16_t GPU::PIXELS_PER_ROW = 160;
const uint16_t GPU::PIXELS_PER_COL = 144;

const uint16_t GPU::TILE_PIXELS_PER_ROW = 8;
const uint16_t GPU::TILE_PIXELS_PER_COL = 8;

GPU::GPU(MemoryController & memory)
    : m_memory(memory),
      m_control(m_memory.read(GPU_CONTROL_ADDRESS)),
      m_status(m_memory.read(GPU_STATUS_ADDRESS)),
      m_x(m_memory.read(GPU_SCROLLX_ADDRESS)),
      m_y(m_memory.read(GPU_SCROLLY_ADDRESS)),
      m_scanline(m_memory.read(GPU_SCANLINE_ADDRESS)),
      m_cpu(nullptr)
{
    reset();
}

void GPU::reset()
{
    m_status = 0x00;
    updateRenderStateStatus(OAM);
    
    m_state = OAM;
    
    m_x = m_y = m_scanline = m_ticks = 0;
}

GPU::RenderState GPU::next()
{
    switch (m_state) {
    case HBLANK: {
        if (HBLANK_TICKS == m_ticks) {
            return (PIXELS_PER_COL == m_scanline) ? VBLANK : OAM;
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
    updateRenderStateStatus(HBLANK);

    if (m_ticks < HBLANK_TICKS) {
        return;
    }

    m_scanline++;
    if (PIXELS_PER_COL == m_scanline) {
        m_cpu->setVBlankInterrupt();
    }
}

void GPU::handleVBlank()
{
    updateRenderStateStatus(VBLANK);

    if (m_ticks < VBLANK_TICKS) {
        return;
    }

    m_scanline = 0;
}

void GPU::handleOAM()
{
    updateRenderStateStatus(OAM);

    if (m_ticks < OAM_TICKS) {
        return;
    }
}

void GPU::handleVRAM()
{
    updateRenderStateStatus(VRAM);

    if (m_ticks < VRAM_TICKS) {
        return;
    }
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
    uint16_t address = offset + ((y * TILE_MAP_COLUMNS) + x);

    // We have the address, so let's read out our actual value and shove it in to a
    // tile pointer object.  This will let us interpret the number that comes out as
    // either a signed integer (in the case of map 1) or an unsigned integer (in the
    // case of map 2).
    union { uint8_t tile0; int8_t tile1; } tptr;
    tptr.tile0 = m_memory.read(address);

    // Now that we have our union initialized, we need to figure out the offset we
    // need to use in order to figure out the location of our tile in memory.
    int16_t toffset = (MAP_0 == index) ?
        int16_t(tptr.tile0) * TILE_SIZE : int16_t(tptr.tile1) * TILE_SIZE;
    
    // Actually figure out the location of the tile that we want.  If we are after a
    // tile in the second set, then we need to set our offset to the end of the 1st
    // set and use our signed number to index in to the second set because the 2nd
    // half of the 1st set is also the 1st half the second set.
    uint16_t location = (MAP_0 == index) ?
            TILE_SET_0_OFFSET + toffset : TILE_SET_0_OFFSET + toffset + (TILES_PER_SET * TILE_SIZE);

    return lookup(location);
}

vector<GPU::RGB> GPU::toRGB(const Tile & tile)
{
    vector<RGB> colors;

    for (size_t i = 0; i < tile.size(); i += 2) {
        uint8_t lower = tile.at(i);
        uint8_t upper = tile.at(i + 1);

        for (uint8_t i = 0; i < 8; i++) {
            // The left most pixel starts at the most significant bit.
            uint8_t shift = 7 - i;

            // Each pixel is spread across two bytes i.e. the least significant bit of
            // the left most pixel is at the most significant bit of the first byte and
            // the most of significant bit of the left most pixel is at the most
            // significant bit of the second byte.
            uint8_t pixel = (((upper >> shift) & 0x01) << 1) | ((lower >> shift) & 0x01);

            // Each pixel needs to be run through a pallette to get the actual color.
            switch (pixel) {
            case 0x03: colors.push_back({ 0, 0, 0, 0xFF }); break;
            case 0x02: colors.push_back({ 96, 96, 96, 0xFF }); break;
            case 0x01: colors.push_back({ 192, 192, 192, 0xFF }); break;
            case 0x00: colors.push_back({ 255, 255, 255, 0xFF }); break;
            default: assert(0); break;
            }
        }
    }

    return colors;
}

vector<GPU::RGB> GPU::lookup(MapIndex index)
{
    // TODO: need to properly handle shifting by the scroll x and scroll y controls
    
    vector<RGB> colors;
    colors.resize(PIXELS_PER_ROW * PIXELS_PER_COL);

    uint8_t xTileOffset = m_x / TILE_PIXELS_PER_ROW;
    uint8_t yTileOffset = m_y / TILE_PIXELS_PER_COL;
    
    // Loop through our map.  The map is 32x32 tiles, but we only can display 20x18 
    // tiles on the screen at one time, so we only need to loop through a portion of
    // our map in order to build the screen.
    for (uint16_t i = 0; i < (PIXELS_PER_COL / TILE_PIXELS_PER_COL); i++) {
        for (uint16_t j = 0; j < (PIXELS_PER_ROW / TILE_PIXELS_PER_ROW); j++) {
            // Lookup the tile that we want and translate it to RGB.  The x and y
            // registers determine the top left corner of the portion of the map that
            // we are drawing, so make sure we offset our row and column by y and x
            // respectively.
            vector<RGB> rgb = toRGB(lookup(index, xTileOffset + j, yTileOffset + i));

            // Each tile is 8x8, so we first need to figure out the coordinates
            // of the top left corner of the tile we are translating.
            uint16_t xOffset = j * TILE_PIXELS_PER_ROW;
            uint16_t yOffset = i * TILE_PIXELS_PER_COL;

            // Loop through each pixel in the tile that we just translated, and stick
            // it in to the screen RGB array at the appropriate pixel location.
            for (size_t k = 0; k < rgb.size(); k++) {
                // These are 8x8 chunks, so we need to calculate a new x, y each time
                // we go through this loop and have a new pixel.
                uint16_t x = xOffset + (k % TILE_PIXELS_PER_ROW);
                uint16_t y = yOffset + (k / TILE_PIXELS_PER_ROW);

                uint64_t index = (y * PIXELS_PER_ROW) + x;
                colors[index] = rgb[k];
            }
        }
    }

    return colors;
}
