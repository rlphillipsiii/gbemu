/*
 * gpu.cpp
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#include <cassert>
#include <vector>
#include <array>
#include <utility>
#include <algorithm>
#include <memory>
#include <sstream>
#include <iomanip>
#include <string>
#include <iostream>

#include "gpu.h"
#include "processor.h"
#include "memorycontroller.h"
#include "memmap.h"
#include "logging.h"
#include "gameboyinterface.h"
#include "iiterator.h"

using std::vector;
using std::array;
using std::pair;
using std::shared_ptr;
using std::stringstream;
using std::string;

const BWPalette GPU::NON_CGB_PALETTE = {{
    { 255, 255, 255, 0xFF }, // white
    { 192, 192, 192, 0xFF }, // light grey
    { 96,  96,  96,  0xFF }, // grey
    { 0,   0,   0,   0xFF }, // black
}};

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

const uint16_t GPU::SPRITE_ATTRIBUTES_TABLE = GPU_SPRITE_TABLE_ADDRESS;

const uint8_t GPU::SPRITE_COUNT = 40;

const uint8_t GPU::SPRITE_BYTES_PER_ATTRIBUTE = 4;

const uint8_t GPU::SPRITE_WIDTH = 8;

const uint8_t GPU::SPRITE_HEIGHT_NORMAL   = 8;
const uint8_t GPU::SPRITE_HEIGHT_EXTENDED = 16;

const uint16_t GPU::OAM_TICKS    = 80;
const uint16_t GPU::VRAM_TICKS   = 172;
const uint16_t GPU::HBLANK_TICKS = 204;
const uint16_t GPU::VBLANK_TICKS = 456;

const uint16_t GPU::SCANLINE_MAX = 155;

const uint16_t GPU::PIXELS_PER_ROW = 160;
const uint16_t GPU::PIXELS_PER_COL = 144;

const uint16_t GPU::TILE_PIXELS_PER_ROW = 8;
const uint16_t GPU::TILE_PIXELS_PER_COL = 8;

const uint8_t GPU::ALPHA_TRANSPARENT = 0x00;

GPU::GPU(MemoryController & memory)
    : m_memory(memory),
      m_control(m_memory.read(GPU_CONTROL_ADDRESS)),
      m_status(m_memory.read(GPU_STATUS_ADDRESS)),
      m_palette(m_memory.read(GPU_PALETTE_ADDRESS)),
      m_sPalette0(m_memory.read(GPU_OBP1_ADDRESS)),
      m_sPalette1(m_memory.read(GPU_OBP2_ADDRESS)),
      m_x(m_memory.read(GPU_SCROLLX_ADDRESS)),
      m_y(m_memory.read(GPU_SCROLLY_ADDRESS)),
      m_scanline(m_memory.read(GPU_SCANLINE_ADDRESS)),
      m_cgb(false)
{
    reset();

    for (size_t i = 0; i < m_sprites.size(); i++) {
        uint16_t address = SPRITE_ATTRIBUTES_TABLE + (i * SPRITE_BYTES_PER_ATTRIBUTE);

        uint8_t & y     = m_memory.read(address);
        uint8_t & x     = m_memory.read(address + 1);
        uint8_t & flags = m_memory.read(address + 3);

        shared_ptr<SpriteData> data(new SpriteData(x, y, flags, m_sPalette0, m_sPalette1));
        assert(data);

        data->address = address;
        data->pointer = GPU_RAM_OFFSET;
        data->height  = SPRITE_HEIGHT_NORMAL;
        
        m_sprites[i] = data;
    }
}

void GPU::reset()
{
    m_control = 0x91;
    m_status  = 0x85;

    updateRenderStateStatus(OAM);
    
    m_state = OAM;
    
    m_x = m_y = m_scanline = m_ticks = 0;
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
            return (isDisplayEnabled() && (SCANLINE_MAX == m_scanline)) ? HBLANK : VBLANK;
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
    
    return (isDisplayEnabled()) ? m_state : VBLANK;
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
        // The hblank mode hasn't been set yet, so this is the first time that we
        // have executed the function in this mode.  If so, we need to figure out
        // if we are coming out of a vblank.  If that is the case, we need to
        // clear the scanline as we are starting back at the beginning.
        if (VBLANK == (m_status & RENDER_MODE)) {
            m_scanline = 0;
        }
        
        if (isHBlankInterruptEnabled()) {
            Interrupts::set(m_memory, InterruptMask::LCD);
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
            Interrupts::set(m_memory, InterruptMask::LCD);
        }

        Interrupts::set(m_memory, InterruptMask::VBLANK);
        updateRenderStateStatus(VBLANK);
    }

    if ((VBLANK_TICKS - 10) <= m_ticks) {
        m_scanline = std::min(uint16_t(m_scanline + 1), SCANLINE_MAX);
    }
}

void GPU::handleOAM()
{
    if (OAM != (m_status & RENDER_MODE)) {
        if (isOAMInterruptEnabled()) {
            Interrupts::set(m_memory, InterruptMask::LCD);
        }
        updateRenderStateStatus(OAM);
    }
}

void GPU::handleVRAM()
{
    if (VRAM != (m_status & RENDER_MODE)) {
        updateRenderStateStatus(VRAM);
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

ColorArray GPU::getColorMap()
{
    //std::string temp;
    //std::getline(std::cin, temp);
    
    if (isWindowEnabled()) {

    } else {

    }

    TileMapIndex mIndex = (m_control & TILE_SET_SELECT) ? TILEMAP_0 : TILEMAP_1;
    TileSetIndex sIndex = (m_control & BACKGROUND_MAP) ? TILESET_1 : TILESET_0;

    return lookup(mIndex, sIndex);
}

Tile GPU::lookup(TileMapIndex mIndex, TileSetIndex sIndex, uint16_t x, uint16_t y)
{
    // We have two maps, so figure out which offset we need to use for our address
    uint16_t offset = (TILEMAP_0 == mIndex) ? TILE_MAP_0_OFFSET : TILE_MAP_1_OFFSET;

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
    int16_t toffset = (TILESET_0 == sIndex) ?
        int16_t(tptr.tile0) * TILE_SIZE : int16_t(tptr.tile1) * TILE_SIZE;
    
    // Actually figure out the location of the tile that we want.  If we are after a
    // tile in the second set, then we need to set our offset to the end of the 1st
    // set and use our signed number to index in to the second set because the 2nd
    // half of the 1st set is also the 1st half the second set.
    uint16_t location = (TILESET_0 == sIndex) ?
            TILE_SET_0_OFFSET + toffset : TILE_SET_0_OFFSET + toffset + (TILES_PER_SET * TILE_SIZE);

    return lookup(location);
}

shared_ptr<GB::RGB> GPU::palette(const uint8_t & pal, uint8_t pixel, bool white) const
{
    if (m_cgb) {
        // TODO: do something with cgb mode here
        assert(0);
    }

    uint8_t index = pixel & 0x03;

    uint8_t color = (pal >> (index * 2)) & 0x03;

    shared_ptr<GB::RGB> rgb(new GB::RGB(NON_CGB_PALETTE[color]));;
    assert(rgb);

    // If the color palette entry is 0, and we are not allowing the color white, then we
    // need to set our alpha blend entry to transparent so that this pixel won't show up
    // on the display when we go to draw the screen.
    if ((0 == color) && !white) {
        rgb->alpha = ALPHA_TRANSPARENT;
    }
    return rgb;
}

ColorArray GPU::toRGB(const uint8_t & pal, const Tile & tile, bool white) const
{
    ColorArray colors;

    for (size_t i = 0; i < tile.size(); i += 2) {
        uint8_t lower = tile.at(i);
        uint8_t upper = tile.at(i + 1);

        for (uint8_t j = 0; j < 8; j++) {
            // The left most pixel starts at the most significant bit.
            uint8_t shift = 7 - j;

            // Each pixel is spread across two bytes i.e. the least significant bit of
            // the left most pixel is at the most significant bit of the first byte and
            // the most of significant bit of the left most pixel is at the most
            // significant bit of the second byte.
            uint8_t pixel = (((upper >> shift) & 0x01) << 1) | ((lower >> shift) & 0x01);

            // Each pixel needs to be run through a palette to get the actual color.
            colors.push_back(std::move(palette(pal, pixel, white)));
        }
    }

    return colors;
}

ColorArray GPU::lookup(TileMapIndex mIndex, TileSetIndex sIndex)
{
    vector<ColorArray> colors;

    // Reserve enough space to fit our 32x32 RGBA pixel matrix.
    colors.resize(TILE_MAP_COLUMNS * TILE_PIXELS_PER_COL);
    for (ColorArray & row : colors) {
        row.resize(TILE_MAP_ROWS * TILE_PIXELS_PER_ROW);
    }

    // Loop through our map.  We can't display the whole screen, but we are going to
    // build the whole map anyway and shift around the screen portion later.
    for (uint16_t i = 0; i < TILE_MAP_ROWS; i++) {
        for (uint16_t j = 0; j < TILE_MAP_COLUMNS; j++) {
            // Lookup the tile that we want and translate it to RGB.
            ColorArray rgb = toRGB(m_palette, lookup(mIndex, sIndex, j, i), true);

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

                colors[y][x] = std::move(rgb[k]);
            }
        }
    }

    ColorArray screen = constrain(colors);
    drawSprites(screen);

    return screen;
}

ColorArray GPU::constrain(const vector<ColorArray> & display) const
{
    uint16_t index = 0;

    uint16_t yTemp = m_y, xTemp = m_x;
    
    // We have an array of the whole map, so now we need to build a flat array of the
    // pixels that we need to display on screen.  The x and y scroll registers determine
    // where in the 32x32 map that we should place the top left corner.
    ColorArray screen(PIXELS_PER_ROW * PIXELS_PER_COL);
    for (uint16_t y = m_y; y < m_y + PIXELS_PER_COL; y++) {
        for (uint16_t x = m_x; x < m_x + PIXELS_PER_ROW; x++) {
            uint16_t yIndex = y % display.size();
            uint16_t xIndex = x % display[yIndex].size();

#ifdef DEBUG
            if ((yIndex >= display.size())
                || (xIndex >= display[yIndex].size())
                || (index >= screen.size())) {
                LOG("Coordinates:   (%d, %d)\n", x, y);
                LOG("Display Index: (%d, %d) ==> xMax %d - yMax %d\n",
                    xIndex, yIndex, int(display.size()), int(display[yIndex].size()));
                LOG("Screen Index:  %d ==> %d\n", index, int(screen.size()));
                assert(0);
            }

            assert((xTemp == m_x) && (yTemp == m_y));
            xTemp = m_x; yTemp = m_y;
#endif

            assert(display[yIndex][xIndex]);
            screen[index++] = std::move(display[yIndex][xIndex]);
        }
    }

    return screen;
}

void GPU::readSprite(SpriteData & data)
{
    // Lookup the tile in our table of sprites.  If we have a sprite that has extended
    // height, then we need to read out the sprite data at the next address as well.
    Tile tile = lookup(data.pointer);
    if (SPRITE_HEIGHT_EXTENDED == data.height) {
        Tile additional = lookup(data.pointer + 1);
        tile.insert(tile.end(), additional.begin(), additional.end());
    }

    ColorArray temp = toRGB(data.palette(), tile, false);

    bool flipX = data.flags & FLIP_X;
    bool flipY = data.flags & FLIP_Y;

    // We already have an RGB color map of the sprite, so if the flip x or y flags aren't
    // set, we don't have anything else that we need to do here.
    if (!(flipX || flipY)) {
        data.colors = std::move(temp);
        return;
    }

    data.colors.clear();
    
    IndexIterator yIndex(0, data.height - 1, flipY);
    while (yIndex.hasNext()) {
        IndexIterator xIndex(0, SPRITE_WIDTH - 1, flipX);

        uint8_t y = yIndex.next();

        while (xIndex.hasNext()) {
            uint8_t x = xIndex.next();

            uint8_t index = ((y * SPRITE_WIDTH) + x);
            data.colors.push_back(std::move(temp.at(index)));
        }
    }
}

void GPU::drawSprites(ColorArray & display)
{
    if (!areSpritesEnabled()) { return; }

    vector<shared_ptr<SpriteData>> enabled;

    for (shared_ptr<SpriteData> & data : m_sprites) {
        if (!data) { continue; }

        data->pointer = TILE_SET_0_OFFSET + (TILE_SIZE * m_memory.read(data->address + 2));

        // Figure out the height of the sprite.  If the height of the sprite, is extended
        // (8x16), then we need to mask out the least significant bit and use the masked
        // address as the upper portion of the sprite and the next sprite tile as the lower
        // portion.
        data->height = (m_control & SPRITE_SIZE) ? SPRITE_HEIGHT_EXTENDED : SPRITE_HEIGHT_NORMAL;
        if (SPRITE_HEIGHT_EXTENDED == data->height) {
            data->pointer &= 0xFE;
        }

        if (data->isVisible()) {
            enabled.push_back(data);
            //break;
        }
    }

    // Sort the list from lowest priority to highest.  This is going to allow us to loop
    // the enabled list in order and draw the pixels in our display array.  Because our
    // highest priority sprites are going to be drawn last, we guarantee that the they are
    // going to be on top.
    //
    // In CGB mode, priority is determined by address (lower address = higher priority).
    // In non-CGB mode, the priority first determined by the x position (farther to the left
    // has higher priority).  If the x position matches, then the address is used to
    // determine the priority (same rule as CGB mode).
    std::sort(enabled.begin(), enabled.end(),
            [this](const shared_ptr<SpriteData> & a, const shared_ptr<SpriteData> & b) {
                return (this->m_cgb || (a->x == b->x)) ? (a->address < b->address) : (a->x < b->x);
        });

    for (const shared_ptr<SpriteData> & sprite : enabled) {
        sprite->render(*this, display, m_palette);
    }
}

////////////////////////////////////////////////////////////////////////////////
GPU::SpriteData::SpriteData(
    const uint8_t & col,
    const uint8_t & row,
    const uint8_t & atts,
    const uint8_t & p0,
    const uint8_t & p1)
    : x(col),
      y(row),
      flags(atts),
      palette0(p0),
      palette1(p1)
{

}

uint8_t GPU::SpriteData::palette() const
{
    return ((this->flags & PALETTE_NUMBER_NON_CGB) ? palette1 : palette0);
}

bool GPU::SpriteData::isVisible() const
{
    if ((0 == this->x) || (0 == this->y)) { return false; }

    if ((this->x >= PIXELS_PER_ROW + 8) || (this->y >= PIXELS_PER_COL + 16)) {
        return false;
    }

    return true;
}

void GPU::SpriteData::render(GPU & gpu, ColorArray & display, uint8_t dPalette)
{
    //std::cout << toString() << std::endl;
    
    gpu.readSprite(*this);

    for (size_t i = 0; i < this->colors.size(); i++) {
        int xOffset = (i % SPRITE_WIDTH) - 8;
        if ((this->x + xOffset) < 0) { continue; }

        int yOffset = (i / SPRITE_WIDTH) - 16;
        if ((this->y + yOffset) < 0) { continue; }

        uint8_t x = this->x + xOffset;
        uint8_t y = this->y + yOffset;

        uint16_t index = (y * PIXELS_PER_ROW) + x;
        if (index >= int(display.size())) {
            continue;
        }

        // Look at the alpha blend and make sure that this sprite pixel isn't supposed to be
        // transparent.  If it is, then we need to leave our display alone.
        const shared_ptr<GB::RGB> & color = this->colors.at(i);
        if (ALPHA_TRANSPARENT == color->alpha) {
            continue;
        }

        // Check the sprite's priority.  If the sprite priority bit is set, the pixels are
        // supposed to be behind the background (i.e. not shown) unless the background pixel
        // that the sprite overlaps with is set to color 0.
        if (this->flags & OBJECT_PRIORITY) {
            const GB::RGB & bg = NON_CGB_PALETTE[dPalette & 0x03];
            if (!(*display[index] == bg)) {
                continue;
            }
        }

        display[index] = color;
    }
}

string GPU::SpriteData::toString() const
{
    stringstream stream;

    stream << "Sprite: " << std::hex << "0x" << this->address << std::endl;
    stream << "    Location: (" << int(this->x) << ", " << int(this->y) << ") " << std::endl;;
    stream << "    Height:   " << int(this->height) << std::endl;
    stream << "    Flags:    " << int(this->flags) << std::endl;
    stream << "    Pointer:  " << this->pointer;
    
    return stream.str();
}
