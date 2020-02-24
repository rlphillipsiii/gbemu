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
#include <cmath>
#include <unordered_map>

#include "gpu.h"
#include "processor.h"
#include "memorycontroller.h"
#include "memmap.h"
#include "logging.h"
#include "gameboyinterface.h"

using std::vector;
using std::array;
using std::pair;
using std::shared_ptr;
using std::stringstream;
using std::string;
using std::unordered_map;
using std::lock_guard;
using std::mutex;

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

const uint8_t GPU::SPRITE_X_OFFSET = 8;
const uint8_t GPU::SPRITE_Y_OFFSET = 16;

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

const uint8_t GPU::WINDOW_ROW_OFFSET = 7;

GPU::GPU(MemoryController & memory)
    : m_memory(memory),
      m_control(m_memory.read(GPU_CONTROL_ADDRESS)),
      m_status(m_memory.read(GPU_STATUS_ADDRESS)),
      m_palette(m_memory.read(GPU_PALETTE_ADDRESS)),
      m_sPalette0(m_memory.read(GPU_OBP1_ADDRESS)),
      m_sPalette1(m_memory.read(GPU_OBP2_ADDRESS)),
      m_x(m_memory.read(GPU_SCROLLX_ADDRESS)),
      m_y(m_memory.read(GPU_SCROLLY_ADDRESS)),
      m_winX(m_memory.read(GPU_WINDOW_X_ADDRESS)),
      m_winY(m_memory.read(GPU_WINDOW_Y_ADDRESS)),
      m_scanline(m_memory.read(GPU_SCANLINE_ADDRESS)),
      m_cgb(false),
      m_screen(PIXELS_PER_ROW * PIXELS_PER_COL),
      m_buffer(PIXELS_PER_ROW * PIXELS_PER_COL)
{
    reset();

    for (size_t i = 0; i < m_sprites.size(); i++) {
        uint16_t address = SPRITE_ATTRIBUTES_TABLE + (i * SPRITE_BYTES_PER_ATTRIBUTE);

        const uint8_t & y     = m_memory.read(address);
        const uint8_t & x     = m_memory.read(address + 1);
        const uint8_t & tile  = m_memory.read(address + 2);
        const uint8_t & flags = m_memory.read(address + 3);

        shared_ptr<SpriteData> data(new SpriteData(*this, x, y, tile, flags, m_sPalette0, m_sPalette1));
        assert(data);

        data->address = address;
        data->height  = SPRITE_HEIGHT_NORMAL;
        
        m_sprites[i] = data;
    }

    for (uint16_t i = 0; i < TILES_PER_SET; i++) {
        uint16_t address = TILE_SET_0_OFFSET + (i * TILE_SIZE);
        for (uint8_t j = 0; j < TILE_SIZE; j++) {
            m_tiles[TILESET_0][i].push_back(&m_memory.read(address + j));
        }

        union { uint8_t uVal; int8_t sVal; } offset = { .uVal = uint8_t(i) };

        address = TILE_SET_0_OFFSET + (TILES_PER_SET * TILE_SIZE) + (offset.sVal * TILE_SIZE);
        for (uint8_t j = 0; j < TILE_SIZE; j++) {
            m_tiles[TILESET_1][i].push_back(&m_memory.read(address + j));
        }
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
        if (m_ticks < HBLANK_TICKS) { break; }

        m_ticks -= HBLANK_TICKS;
        return (PIXELS_PER_COL == m_scanline) ? VBLANK : OAM;
    }
    case VBLANK: {
        if (m_ticks < VBLANK_TICKS) { break; }

        m_ticks -= VBLANK_TICKS;
        return (SCANLINE_MAX == m_scanline) ? HBLANK : VBLANK;
    }
    case OAM: {
        if (m_ticks < OAM_TICKS) { break; }

        m_ticks -= OAM_TICKS;
        return VRAM;
    }
    case VRAM: {
        if (m_ticks < VRAM_TICKS) { break; }

        m_ticks -= VRAM_TICKS;
        return HBLANK;
    }
    default: {
        LOG("GPU::next() : Unknown render state %d\n", m_state);
    
        assert(0);
        break;
    }
    }
    
    return m_state;
}

void GPU::cycle(uint8_t ticks)
{
    if (!isDisplayEnabled()) { return; }
    
    m_ticks += ticks;

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
    
    m_state = next();
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

    if (m_ticks < HBLANK_TICKS) { return; }

    updateScreen();
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

        {
            lock_guard<mutex> guard(m_lock);
            m_screen = std::move(m_buffer);
        }
        m_buffer.resize(PIXELS_PER_ROW * PIXELS_PER_COL);
    }

    const int cycles = VBLANK_TICKS / (SCANLINE_MAX - PIXELS_PER_COL);
    if (0 != m_ticks%cycles) {
        return;
    }

    m_scanline = std::min(uint16_t(m_scanline + 1), SCANLINE_MAX);
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

void GPU::updateScreen()
{
    if (m_scanline >= PIXELS_PER_COL) { return; }
    
    TileSetIndex set = (m_control & TILE_SET_SELECT) ? TILESET_0 : TILESET_1;
    
    TileMapIndex background = (m_control & BACKGROUND_MAP) ? TILEMAP_1 : TILEMAP_0;
    TileMapIndex window     = (m_control & WINDOW_MAP) ? TILEMAP_1 : TILEMAP_0;

    draw(set, background, window);
}

const Tile & GPU::lookup(TileMapIndex mIndex, TileSetIndex sIndex, uint16_t x, uint16_t y)
{
    // We have two maps, so figure out which offset we need to use for our address
    uint16_t offset = (TILEMAP_0 == mIndex) ? TILE_MAP_0_OFFSET : TILE_MAP_1_OFFSET;
    
    // Now that we know our offset, figure out the address of the tile that we are
    // needing to look up.
    uint16_t address = offset + ((y * TILE_MAP_COLUMNS) + x);

    return getTile(sIndex, m_memory.read(address));
}

shared_ptr<GB::RGB> GPU::palette(const uint8_t & pal, uint8_t pixel, bool white) const
{
    // TODO: do something with cgb mode here
    if (m_cgb) { assert(0); }

    uint8_t index = pixel & 0x03;
    uint8_t color = (pal >> (index * 2)) & 0x03;

    shared_ptr<GB::RGB> rgb(new GB::RGB(NON_CGB_PALETTE[color]));;
    assert(rgb);

    // If the color palette entry is 0, and we are not allowing the color white, then we
    // need to set our alpha blend entry to transparent so that this pixel won't show up
    // on the display when we go to draw the screen.
    if ((0 == index) && !white) {
        rgb->alpha = ALPHA_TRANSPARENT;
    }
    return rgb;
}

ColorArray GPU::toRGB(
    const uint8_t & pal,
    const Tile & tile,
    uint8_t row,
    bool white,
    bool flip) const
{
    ColorArray colors;

    uint8_t offset = row * 2;

    uint8_t lower = *tile.at(offset);
    uint8_t upper = *tile.at(offset + 1);

    for (uint8_t j = 0; j < 8; j++) {
        uint8_t index = (flip) ? 7 - j : j;

        // The left most pixel starts at the most significant bit.
        uint8_t shift = 7 - index;

        // Each pixel is spread across two bytes i.e. the least significant bit of
        // the left most pixel is at the most significant bit of the first byte and
        // the most of significant bit of the left most pixel is at the most
        // significant bit of the second byte.
        uint8_t pixel = (((upper >> shift) & 0x01) << 1) | ((lower >> shift) & 0x01);

        // Each pixel needs to be run through a palette to get the actual color.
        colors.push_back(palette(pal, pixel, white));
    }

    return colors;
}

bool GPU::isWindowSelected(uint8_t x, uint8_t y)
{
    // We are trying to figure out if the window is visible at the given x and y,
    // so the first thing that we need to check is if the window is even enabled.
    if (!isWindowEnabled()) { return false; }
    
    if ((y < m_winY) || (y >= PIXELS_PER_COL)) { return false; }

    if (((x + WINDOW_ROW_OFFSET) < m_winX)
        || (x >= (PIXELS_PER_ROW + WINDOW_ROW_OFFSET))) { return false; }

    return true;
}

void GPU::drawBackground(TileSetIndex set, TileMapIndex background, TileMapIndex window)
{
    if (!isBackgroundEnabled()) { return; }
    
    unordered_map<uint16_t, ColorArray> winCache;
    unordered_map<uint16_t, ColorArray> bgCache;
    
    uint16_t offset = m_scanline * PIXELS_PER_ROW;

    for (uint8_t pixel = 0; pixel < PIXELS_PER_ROW; pixel++) {
        bool win = isWindowSelected(pixel, m_scanline);
        
        // Figure out the row and column that we're actually talking about and
        // make sure that if the pixels walk off the edge of our background map
        // that we wrap back around.
        uint16_t col = (m_x + pixel);
        uint16_t row = (m_y + m_scanline);

        // If the window is active, we need to adjust our lookup locations for
        // where the top left location of the window is located.
        if (win) {
            col -= (m_winX - WINDOW_ROW_OFFSET);
            row -= m_winY;
        }
        
        col %= (TILE_MAP_ROWS * TILE_PIXELS_PER_ROW);
        row %= (TILE_MAP_ROWS * TILE_PIXELS_PER_ROW);

        // Figure out the coordinates of the tile that we need to look up in
        // order to draw the pixels that we are after, which may or may not be
        // the whole row from this tile.
        uint16_t xOffset = col / TILE_PIXELS_PER_ROW;
        uint16_t yOffset = row / TILE_PIXELS_PER_COL;

        auto & cache = (win) ? winCache : bgCache;

        // First we need to see if we've previously looked up this tile.  If not,
        // we need to look this tile up and stick it in our cache.
        auto iterator = cache.find(xOffset);
        if (cache.end() == iterator) {
            const Tile & tile = lookup(((win) ? window : background), set, xOffset, yOffset);

            // Now that we have the tile that we are interested in, we need to get
            // the RGB values that are associated with the row in the tile that we
            // need to add to our screen buffer.
            ColorArray rgb = toRGB(m_palette, tile, row % TILE_PIXELS_PER_COL, true, false);

            auto entry = cache.emplace(xOffset, rgb);
            iterator = entry.first;
        }

        ColorArray & rgb = iterator->second;
        
        uint16_t index = col - (xOffset * TILE_PIXELS_PER_ROW);

        // Write this pixel to the the screen buffer.  Its location in the
        // row that we are drawing is the background x offset subtracted
        // from the pixel location and then modded by the pixels per
        // row on screen to wrap the background back around to the left
        // side of the screen.
        uint16_t pX = (xOffset * TILE_PIXELS_PER_ROW) + index;
        if (pX < m_x) { pX += m_x; }
        
        m_buffer[offset + ((pX - m_x) % PIXELS_PER_ROW)] = std::move(rgb[index]);
    }
}

void GPU::draw(TileSetIndex set, TileMapIndex background, TileMapIndex window)
{
    // Draw the 3 layers of the screen in order from lowest priority to highest
    // so that the highest priority pixels actually end up on top.  The special
    // priority states of the sprites will be handled by the sprite drawing
    // routine itself.

    // Background -> Window -> Sprites
    drawBackground(set, background, window);
    drawSprites(m_buffer);
}

void GPU::readSprite(SpriteData & data)
{
    // If the sprite height is extended, then we need to mask out the lower bit and
    // take that as upper tile in the sprite.  The new masked tile number + 1 is the
    // address of the lower number.
    uint8_t number = data.tile;
    if (SPRITE_HEIGHT_EXTENDED == data.height) {
        number &= 0xFE;
    }
    
    // Lookup the tile in our table of sprites.  If we have a sprite that has extended
    // height, then we need to read out the sprite data at the next address as well.
    Tile tile = getTile(TILESET_0, number);
    if (SPRITE_HEIGHT_EXTENDED == data.height) {
        const Tile & additional = getTile(TILESET_0, number + 1);
        tile.insert(tile.end(), additional.begin(), additional.end());
    }

    bool flipX = data.flags & FLIP_X;
    bool flipY = data.flags & FLIP_Y;

    data.colors.clear();

    // TODO: This isn't quite right here.  Fix it.
    if (data.y < SPRITE_Y_OFFSET) { return; }
    uint8_t row = m_scanline - (data.y - SPRITE_Y_OFFSET);
    
    // Figure out which row we are trying to render
    //uint8_t row = m_scanline % TILE_PIXELS_PER_COL;
    if (flipY) { row = TILE_PIXELS_PER_COL - row - 1; }
    
    data.colors = toRGB(data.palette(), tile, row, false, flipX);
}

void GPU::drawSprites(ColorArray & display)
{
    if (!areSpritesEnabled()) { return; }

    vector<shared_ptr<SpriteData>> enabled;

    for (shared_ptr<SpriteData> & data : m_sprites) {
        if (!data) { continue; }

        data->height = (m_control & SPRITE_SIZE) ? SPRITE_HEIGHT_EXTENDED : SPRITE_HEIGHT_NORMAL;
        if (data->isVisible()) {
            enabled.push_back(data);
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
        sprite->render(display, m_palette);
    }
}

////////////////////////////////////////////////////////////////////////////////
GPU::SpriteData::SpriteData(
    GPU & gpu,                            
    const uint8_t & col,
    const uint8_t & row,
    const uint8_t & t,
    const uint8_t & atts,
    const uint8_t & p0,
    const uint8_t & p1)
    : m_gpu(gpu),
      x(col),
      y(row),
      flags(atts),
      palette0(p0),
      palette1(p1),
      tile(t)
{

}

uint8_t GPU::SpriteData::palette() const
{
    return ((this->flags & PALETTE_NUMBER_NON_CGB) ? palette1 : palette0);
}

bool GPU::SpriteData::isVisible() const
{
    // If either coordinates are set to 0 or the sprite is completely off screen
    // then it isn't visible.
    if ((0 == this->x) || (0 == this->y)) { return false; }

    if (this->x >= (PIXELS_PER_ROW + SPRITE_X_OFFSET)) { return false; }
    if (this->y >= (PIXELS_PER_COL + SPRITE_Y_OFFSET)) { return false; }

    // Figure out the y coordinate.  Make sure to clamp the value to 0 if subtracting
    // the Y offset value for a sprite would cause the value to wrap around.  The
    // sprite is visible on the scanline if the scanline is between the y coordinate
    // and the y coordinate plus the height of the sprite.
    uint8_t y = (this->y < SPRITE_Y_OFFSET) ? 0 : this->y - SPRITE_Y_OFFSET;
    if ((m_gpu.m_scanline >= y) && (m_gpu.m_scanline < (y + this->height))) {
        return true;
    }
    
    return false;
}

void GPU::SpriteData::render(ColorArray & display, uint8_t dPalette)
{
    m_gpu.readSprite(*this);

    for (size_t i = 0; i < this->colors.size(); i++) {
        // Look at the alpha blend and make sure that this sprite pixel isn't supposed to be
        // transparent.  If it is, then we need to leave our display alone.
        const shared_ptr<GB::RGB> & color = this->colors.at(i);
        if (ALPHA_TRANSPARENT == color->alpha) { continue; }

        // Figure out the pixel that we are actually after.  If that pixel wrapped around,
        // then it's off screen, and we need to skip it.
        uint8_t x = this->x + i - SPRITE_X_OFFSET;
        if (x >= PIXELS_PER_ROW) { continue; }

        // Grab the actual index in to the display buffer.
        uint16_t index = (m_gpu.m_scanline * PIXELS_PER_ROW) + x;
        assert(index < display.size());

        // Check the sprite's priority.  If the sprite priority bit is set, the pixels are
        // supposed to be behind the background (i.e. not shown) unless the background pixel
        // that the sprite overlaps with is set to color 0.
        if (this->flags & OBJECT_PRIORITY) {
            const GB::RGB & bg = NON_CGB_PALETTE[dPalette & 0x03];
            if (display[index] && !(*display[index] == bg)) {
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
    stream << "    Tile:     " << this->tile << std::endl;
    stream << "    Location: (" << int(this->x) << ", " << int(this->y) << ") " << std::endl;;
    stream << "    Height:   " << int(this->height) << std::endl;
    stream << "    Flags:    " << int(this->flags);
    
    return stream.str();
}
