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

/** Tile map 1 comes directly after the end of tile map 0 */
const uint16_t GPU::TILE_MAP_1_OFFSET = TILE_MAP_0_OFFSET + (TILE_MAP_ROWS * TILE_MAP_COLUMNS);

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
      m_scanline(m_memory.read(GPU_SCANLINE_ADDRESS))
{
    reset();

    LOG("TILE SET 0: 0x%04x\n", TILE_SET_0_OFFSET);
    LOG("TILE SET 1: 0x%04x\n", TILE_SET_1_OFFSET);
    LOG("TILE MAP 0: 0x%04x\n", TILE_MAP_0_OFFSET);
    LOG("TILE_MAP 1: 0x%04x\n", TILE_MAP_1_OFFSET);
}

void GPU::reset()
{
    m_status = 0x00;
    updateRenderStateStatus(OAM);
    
    m_state = OAM;
    
    m_x = m_y = m_scanline = m_ticks = 0;
}

void GPU::setInterrupt(InterruptMask interrupt)
{
    uint8_t & current = m_memory.read(INTERRUPT_FLAGS_ADDRESS);

    uint8_t enabled = m_memory.read(INTERRUPT_MASK_ADDRESS);
    if (enabled & uint8_t(interrupt)) {
        current |= uint8_t(interrupt);
    }
}

GPU::RenderState GPU::next()
{
    switch (m_state) {
    case HBLANK: {
        if (HBLANK_TICKS == m_ticks) {
            if (!isDisplayEnabled()) {
                return VBLANK;
            }
            return (PIXELS_PER_COL == m_scanline) ? VBLANK : OAM;
        }
        break;
    }
    case VBLANK: {
        if (VBLANK_TICKS == m_ticks) {
            return (isDisplayEnabled()) ? HBLANK : VBLANK;
        }
        break;
    }
    case OAM: {
        if (OAM_TICKS == m_ticks) {
            return (isDisplayEnabled()) ? VRAM : VBLANK;
        }
        break;
    }
    case VRAM: {
        if (VRAM_TICKS == m_ticks) {
            return (isDisplayEnabled()) ? HBLANK : VBLANK;
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

    // If the display is turned off or we just changed states, then we need to
    // reset the tick counter back to 0.
    if (!isDisplayEnabled() || (current != m_state)) {
        m_ticks = 0;
    }
}

void GPU::handleHBlank()
{
    if (HBLANK != (m_status & RENDER_MODE)) {
        if (isHBlankInterruptEnabled()) {
            setInterrupt(InterruptMask::LCD);
        }
        updateRenderStateStatus(HBLANK);
    }

    if (m_ticks < HBLANK_TICKS) {
        return;
    }

    m_scanline++;
}

void GPU::handleVBlank()
{
    if (VBLANK != (m_status & RENDER_MODE)) {
        if (isVBlankInterruptEnabled()) {
            setInterrupt(InterruptMask::VBLANK);
        }
        updateRenderStateStatus(VBLANK);
    }

    if (m_ticks < VBLANK_TICKS) {
        return;
    }

    m_scanline = 0;
}

void GPU::handleOAM()
{
    if (OAM != (m_status & RENDER_MODE)) {
        if (isOAMInterruptEnabled()) {
            setInterrupt(InterruptMask::LCD);
        }
        updateRenderStateStatus(OAM);
    }

    if (m_ticks < OAM_TICKS) {
        return;
    }
}

void GPU::handleVRAM()
{
    if (VRAM != (m_status & RENDER_MODE)) {
        updateRenderStateStatus(VRAM);
    }

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

vector<GPU::RGB> GPU::getColorMap()
{
    if (isWindowEnabled()) {

    } else {

    }

    MapIndex index = (m_control & (1 << TILE_SET_SELECT)) ? MAP_0 : MAP_1;

    return lookup(index);
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

GPU::RGB GPU::pallette(uint8_t pixel) const
{
    switch (pixel) {
    case 0x03: return { 0,   0,   0,   0xFF };
    case 0x02: return { 96,  96,  96,  0xFF };
    case 0x01: return { 192, 192, 192, 0xFF };
    case 0x00: return { 255, 255, 255, 0xFF };
    default: assert(0); break;
    }

    return { 0, 0, 0, 0xFF };
}

vector<GPU::RGB> GPU::toRGB(const Tile & tile) const
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
            colors.push_back(pallette(pixel));
        }
    }

    return colors;
}

vector<GPU::RGB> GPU::lookup(MapIndex index)
{
    vector<vector<RGB>> colors;

    // Reserve enough space to fit our 32x32 RGBA pixel matrix.
    colors.resize(TILE_MAP_COLUMNS * TILE_PIXELS_PER_COL);
    for (vector<RGB> & row : colors) {
        row.resize(TILE_MAP_ROWS * TILE_PIXELS_PER_ROW);
    }

    // Loop through our map.  We can't display the whole screen, but we are going to
    // build the whole map anyway and shift around the screen portion later.
    for (uint16_t i = 0; i < TILE_MAP_ROWS; i++) {
        for (uint16_t j = 0; j < TILE_MAP_COLUMNS; j++) {
            // Lookup the tile that we want and translate it to RGB.
            vector<RGB> rgb = toRGB(lookup(index, j, i));

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

                colors[y][x] = rgb[k];
            }
        }
    }

    return constrain(colors);
}

vector<GPU::RGB> GPU::constrain(const vector<vector<RGB>> & display) const
{
    uint16_t index = 0;

    // We have an array of the whole map, so now we need to build a flat array of the
    // pixels that we need to display on screen.  The x and y scroll registers determine
    // where in the 32x32 map that we should place the top left corner.
    vector<RGB> screen(PIXELS_PER_ROW * PIXELS_PER_COL);
    for (uint16_t y = m_y; y < m_y + PIXELS_PER_COL; y++) {
        for (uint16_t x = m_x; x < m_x + PIXELS_PER_ROW; x++) {
            screen[index++] = display[y][x];
        }
    }

    return screen;
}
