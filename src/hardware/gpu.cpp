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

const ColorPalette GPU::DMR_PALETTE = {{
    { { 0, 0 }, { 255, 255, 255, 0xFF } }, // white
    { { 0, 0 }, { 192, 192, 192, 0xFF } }, // light grey
    { { 0, 0 }, { 96,  96,  96,  0xFF } }, // grey
    { { 0, 0 }, { 0,   0,   0,   0xFF } }, // black
}};

const uint8_t GPU::BANK_COUNT = GPU_BANK_COUNT;

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

const uint16_t GPU::BG_ATTRIBUTES_TABLE = TILE_SET_1_OFFSET + (TILES_PER_SET * TILE_SIZE);

const uint16_t GPU::SPRITE_ATTRIBUTES_TABLE = GPU_SPRITE_TABLE_ADDRESS;

const uint8_t GPU::SPRITE_COUNT = 40;

const uint8_t GPU::SPRITE_BYTES_PER_ATTRIBUTE = 4;

const uint8_t GPU::SPRITE_WIDTH = 8;

const uint8_t GPU::SPRITE_HEIGHT_NORMAL   = 8;
const uint8_t GPU::SPRITE_HEIGHT_EXTENDED = 16;

const uint8_t GPU::SPRITE_X_OFFSET = 8;
const uint8_t GPU::SPRITE_Y_OFFSET = 16;

const uint8_t GPU::SPRITE_CGB_PALETTE_COUNT = 7;
const uint8_t GPU::SPRITE_DMR_PALETTE_COUNT = 2;

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
    : MemoryRegion(memory, GPU_RAM_SIZE, GPU_RAM_OFFSET, BANK_COUNT - 1),
      m_mmc(memory),
      m_control(m_mmc.read(GPU_CONTROL_ADDRESS)),
      m_status(m_mmc.read(GPU_STATUS_ADDRESS)),
      m_palette(m_mmc.read(GPU_PALETTE_ADDRESS)),
      m_x(m_mmc.read(GPU_SCROLLX_ADDRESS)),
      m_y(m_mmc.read(GPU_SCROLLY_ADDRESS)),
      m_winX(m_mmc.read(GPU_WINDOW_X_ADDRESS)),
      m_winY(m_mmc.read(GPU_WINDOW_Y_ADDRESS)),
      m_scanline(m_mmc.read(GPU_SCANLINE_ADDRESS)),
      m_screen(PIXELS_PER_ROW * PIXELS_PER_COL),
      m_buffer(PIXELS_PER_ROW * PIXELS_PER_COL)
{
    reset();

    initSpriteCache();
    initTileCache();
}

void GPU::initSpriteCache()
{
    for (size_t i = 0; i < m_sprites.size(); i++) {
        uint16_t address = SPRITE_ATTRIBUTES_TABLE + (i * SPRITE_BYTES_PER_ATTRIBUTE);

        const uint8_t & y     = m_mmc.read(address);
        const uint8_t & x     = m_mmc.read(address + 1);
        const uint8_t & tile  = m_mmc.read(address + 2);
        const uint8_t & flags = m_mmc.read(address + 3);

        shared_ptr<SpriteData> data = std::make_shared<SpriteData>(*this, x, y, tile, flags);

        data->mono = { &m_mmc.read(GPU_OBP1_ADDRESS), &m_mmc.read(GPU_OBP2_ADDRESS) };

        data->address = address;
        data->height  = SPRITE_HEIGHT_NORMAL;

        m_sprites[i] = data;
    }
}

void GPU::initTileCache()
{
    for (uint16_t i = 0; i < TILES_PER_SET; i++) {
        union { uint8_t uVal; int8_t sVal; } offset = { .uVal = uint8_t(i) };

        uint16_t i0 = (TILE_SET_0_OFFSET + (offset.uVal * TILE_SIZE)) - m_offset;
        uint16_t i1 =
            (TILE_SET_0_OFFSET + (TILES_PER_SET * TILE_SIZE) + (offset.sVal * TILE_SIZE))
            - m_offset;

        for (uint8_t j = 0; j < TILE_SIZE; j++) {
            m_tiles[BANK_0][TILESET_0][i].push_back(&read(BANK_0, i0 + j));
            m_tiles[BANK_1][TILESET_0][i].push_back(&read(BANK_1, i0 + j));

            m_tiles[BANK_0][TILESET_1][i].push_back(&read(BANK_0, i1 + j));
            m_tiles[BANK_1][TILESET_1][i].push_back(&read(BANK_1, i1 + j));
        }
    }
}

void GPU::reset()
{
    MemoryRegion::reset();

    m_control = 0x91;
    m_status  = 0x85;

    updateRenderStateStatus(OAM);

    m_state = OAM;

    m_x = m_y = m_scanline = m_ticks = 0;
}

void GPU::write(uint16_t address, uint8_t value)
{
    MemoryBank selected = (MemoryBank)(m_mmc.read(GPU_BANK_SELECT_ADDRESS) & 0x01);
    write(selected, address - m_offset, value);
}

void GPU::write(GPU::MemoryBank selected, uint16_t index, uint8_t value)
{
    m_memory[int(selected)][index] = value;
}

uint8_t & GPU::read(uint16_t address)
{
    MemoryBank selected = (MemoryBank)(m_mmc.read(GPU_BANK_SELECT_ADDRESS) & 0x01);
    return read(selected, address - m_offset);
}

uint8_t & GPU::read(GPU::MemoryBank selected, uint16_t index)
{
    return m_memory[int(selected)][index];
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
            Interrupts::set(m_mmc, InterruptMask::LCD);
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
            Interrupts::set(m_mmc, InterruptMask::LCD);
        }

        Interrupts::set(m_mmc, InterruptMask::VBLANK);
        updateRenderStateStatus(VBLANK);

        // We've move in to the hblank state, so move the internal buffer
        // over to the screen buffer that the external code can retrieve.
        // This empties out our buffer, so we need to resize it to the size
        // of the screen.
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
            Interrupts::set(m_mmc, InterruptMask::LCD);
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

pair<const uint8_t&, const Tile&> GPU::lookup(
    TileMapIndex mIndex,
    TileSetIndex sIndex,
    uint16_t x,
    uint16_t y)
{
    // We have two maps, so figure out which offset we need to use for our address
    uint16_t offset = (TILEMAP_0 == mIndex) ? TILE_MAP_0_OFFSET : TILE_MAP_1_OFFSET;

    // Now that we know our offset, figure out the address of the tile that we are
    // needing to look up.
    uint16_t address = offset + ((y * TILE_MAP_COLUMNS) + x);

    uint8_t & attributes = read(BANK_1, address - m_offset);

    MemoryBank bank = BANK_0;
    if (m_mmc.isCGB()) {
        bank = (attributes & BG_TILE_BANK) ? BANK_1 : BANK_0;
    }

    return { attributes, getTile(bank, sIndex, read(address)) };
}

GB::RGB GPU::palette(
    const ColorPalette & colors,
    uint8_t pal,
    uint8_t pixel,
    bool white) const
{
    uint8_t index = pixel & 0x03;
    uint8_t color = (pal >> (index * 2)) & 0x03;

    GB::RGB rgb = colors.at(color).second;

    // If the color palette entry is 0, and we are not allowing the color white, then we
    // need to set our alpha blend entry to transparent so that this pixel won't show up
    // on the display when we go to draw the screen.
    if ((0 == index) && !white) {
        rgb.alpha = ALPHA_TRANSPARENT;
    }
    return rgb;
}

ColorArray GPU::toRGB(
    const ColorPalette & rgb,
    uint8_t pal,
    const Tile & tile,
    uint8_t row,
    bool white,
    bool flip) const
{
    ColorArray colors;
    colors.reserve(TILE_PIXELS_PER_ROW);

    uint8_t offset = row * 2;

    uint8_t lower = *tile.at(offset);
    uint8_t upper = *tile.at(offset + 1);

    for (uint8_t j = 0; j < uint8_t(colors.capacity()); j++) {
        uint8_t index = (flip) ? 7 - j : j;

        // The left most pixel starts at the most significant bit.
        uint8_t shift = 7 - index;

        // Each pixel is spread across two bytes i.e. the least significant bit of
        // the left most pixel is at the most significant bit of the first byte and
        // the most of significant bit of the left most pixel is at the most
        // significant bit of the second byte.
        uint8_t pixel = (((upper >> shift) & 0x01) << 1) | ((lower >> shift) & 0x01);

        // Each pixel needs to be run through a palette to get the actual color.
        colors.emplace_back(palette(rgb, pal, pixel, white));
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

    struct Cache { uint16_t x; ColorArray rgb; };
    Cache winCache = { .x = 0xFFFF, .rgb = {} }, bgCache = { .x = 0xFFFF, .rgb = {} };

    uint16_t offset = m_scanline * PIXELS_PER_ROW;

    for (uint8_t pixel = 0; pixel < PIXELS_PER_ROW; pixel++) {
        bool win = isWindowSelected(pixel, m_scanline);

        // Figure out the row and column that we're actually talking about and
        // make sure that if the pixels walk off the edge of our background map
        // that we wrap back around.
        uint16_t col = win ? (pixel - m_winX + WINDOW_ROW_OFFSET) : (m_x + pixel);
        uint16_t row = win ? (m_scanline - m_winY) : (m_y + m_scanline);

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
        if (cache.x != xOffset) {
            cache.x = xOffset;

            const auto & [atts, tile] =
                lookup(((win) ? window : background), set, xOffset, yOffset);

            auto & [palettes, colors] = m_palettes.bg;
            uint8_t index = atts & BG_PALETTE_NUMBER;

            uint8_t palette          = (m_mmc.isCGB()) ? palettes.at(index) : m_palette;
            const ColorPalette & rgb = (m_mmc.isCGB()) ? colors.at(index)   : DMR_PALETTE;

            // Now that we have the tile that we are interested in, we need to get
            // the RGB values that are associated with the row in the tile that we
            // need to add to our screen buffer.
            cache.rgb = toRGB(rgb, palette, tile, row % TILE_PIXELS_PER_COL, true, false);
        }

        // Move the RGB values from the iterator in to our buffer.
        m_buffer[offset + pixel] = cache.rgb[col % TILE_PIXELS_PER_ROW];
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

    // Figure out which VRAM bank the sprite tile is sitting in.  If we aren't in
    // CGB mode, then we always need to use bank 0.
    MemoryBank bank = BANK_0;
    if (m_mmc.isCGB()) {
        bank = (data.flags & TILE_BANK_CGB) ? BANK_1 : BANK_0;
    }

    // Lookup the tile in our table of sprites.  If we have a sprite that has extended
    // height, then we need to read out the sprite data at the next address as well.
    Tile tile = getTile(bank, TILESET_0, number);
    if (SPRITE_HEIGHT_EXTENDED == data.height) {
        const Tile & additional = getTile(bank, TILESET_0, number + 1);
        tile.insert(tile.end(), additional.begin(), additional.end());
    }

    bool flipX = data.flags & FLIP_X;
    bool flipY = data.flags & FLIP_Y;

    // Figure out which row we are trying to render.
    uint8_t row = m_scanline - (data.y - SPRITE_Y_OFFSET);
    if (data.y < SPRITE_Y_OFFSET) {
        // Special case for when the top of a sprite is off the screen.  If the sprite
        // isn't partially on screen, then we can bail here without actually looking
        // up the RGB values.
        row = SPRITE_Y_OFFSET - data.y + m_scanline;
        if (row >= data.height) { return; }
    }

    if (flipY) { row = TILE_PIXELS_PER_COL - row - 1; }

    const auto & [palette, colors] = data.palette();
    data.colors = toRGB(colors, palette, tile, row, false, flipX);
}

void GPU::drawSprites(ColorArray & display)
{
    if (!areSpritesEnabled()) { return; }

    vector<shared_ptr<SpriteData>> enabled;
    enabled.reserve(m_sprites.size());

    for (shared_ptr<SpriteData> & data : m_sprites) {
        if (!data) { continue; }

        data->height = (m_control & SPRITE_SIZE) ? SPRITE_HEIGHT_EXTENDED : SPRITE_HEIGHT_NORMAL;
        if (data->isVisible()) {
            enabled.emplace_back(data);
        }
    }

    // Sort the list from lowest priority to highest.  This is going to allow us to loop
    // the enabled list in order and draw the pixels in our display array.  Because our
    // highest priority sprites are going to be drawn last, we guarantee that the they are
    // going to be on top.
    //
    // In CGB mode, priority is determined by address (lower address = higher priority).
    // In non-CGB mode, the priority first determined by the x position (farther to the left
    // Has higher priority).  If the x position matches, then the address is used to
    // determine the priority (same rule as CGB mode).
    std::sort(enabled.begin(), enabled.end(),
            [this](const shared_ptr<SpriteData> & a, const shared_ptr<SpriteData> & b) {
                return (this->m_mmc.isCGB() || (a->x == b->x))
                    ? (a->address < b->address) : (a->x < b->x);
        });

    for (const shared_ptr<SpriteData> & sprite : enabled) {
        sprite->render(display, m_palette);
    }
}

void GPU::writeBgPalette(uint8_t index, uint8_t value)
{
    writePalette(m_palettes.bg, index, value);
}

void GPU::writeSpritePalette(uint8_t index, uint8_t value)
{
    writePalette(m_palettes.sprite, index & 0x3F, value);
}

void GPU::writePalette(CgbPaletteMemory & palette, uint8_t index, uint8_t value)
{
    // The palette that is being passed in is made up of a pair of values.  The first
    // is the palette byte that maps the colors and the second is the array of actual
    // RGB values.
    auto & [palettes, colors] = palette;

    // Figure out which area of the palette memory we need to write to.  The first 8
    // bytes in the memory make up the color palettes.  The rest of the memory is made
    // up of byte pairs that define the actual color RGB values.
    if (index < GPU_CGB_PALETTE_COUNT) {
        palettes[index] = value;
    } else {
        uint8_t idx = index - GPU_CGB_PALETTE_COUNT;

        uint8_t pIdx = idx / GPU_COLORS_PER_PALETTE;
        uint8_t cIdx = idx % GPU_COLORS_PER_PALETTE;

        // The RGB values are held in memory in two different formats.  The first
        // format is the 2 bytes that define the the mask corresponding to the RGB
        // values.  The second is the translation of that mask to the RGB8888 value.
        auto & [bytes, rgb] = colors[pIdx][cIdx];

        // Set the requested byte to the value that was passed in.
        bytes[idx & 0x01] = value;

        // Define a lambda function to convert the mask to its RGB value.  The values
        // stored in the mask are RGB555, so we need to normalize each value to 1,
        // and then apply that normalized number to an 8 bit RGB value to get the
        // value that we are going to store in our RGB8888 structure.
        auto convert = [](uint8_t value) {
            double normalized = double(value) / 31.;
            return uint8_t(0xFF * normalized);
        };

        // Sticking the two bytes in to a 16 bit number makes the color extraction
        // easier.
        uint16_t bits = (bytes[1] << 8) | bytes[0];

        // Extract each color and convert it to RGB8888.
        // RGB555 format (D = don't care): DBBBBBGGGGGRRRRR
        rgb.red   = convert(bits & 0x1F);
        rgb.green = convert((bits >> 5) & 0x1F);
        rgb.blue  = convert((bits >> 10) & 0x1F);
        rgb.alpha = 0xFF;
    }
}

uint8_t & GPU::readBgPalette(uint8_t index)
{
    return readPalette(m_palettes.bg, index);
}

uint8_t & GPU::readSpritePalette(uint8_t index)
{
    return readPalette(m_palettes.sprite, index);
}

uint8_t & GPU::readPalette(CgbPaletteMemory & palette, uint8_t index)
{
    // The palette that is being passed in is made up of a pair of values.  The first
    // is the palette byte that maps the colors and the second is the array of actual
    // RGB values.
    auto & [palettes, colors] = palette;

    // Figure out which area of the palette memory we need to write to.  The first 8
    // bytes in the memory make up the color palettes.  The rest of the memory is made
    // up of byte pairs that define the actual color RGB values.
    if (index < GPU_CGB_PALETTE_COUNT) {
        return palettes[index];
    }

    uint8_t idx = index - GPU_CGB_PALETTE_COUNT;

    uint8_t pIdx = idx / GPU_COLORS_PER_PALETTE;
    uint8_t cIdx = idx % GPU_COLORS_PER_PALETTE;

    // The RGB values are held in memory in two different formats.  The first
    // format is the 2 bytes that define the the mask corresponding to the RGB
    // values.  The second is the translation of that mask to the RGB8888 value.
    auto & [bytes, rgb] = colors[pIdx][cIdx];
    (void)rgb;

    return bytes[idx & 0x01];
}

////////////////////////////////////////////////////////////////////////////////
GPU::SpriteData::SpriteData(
    GPU & gpu,
    const uint8_t & col,
    const uint8_t & row,
    const uint8_t & t,
    const uint8_t & atts)
    : m_gpu(gpu),
      x(col),
      y(row),
      flags(atts),
      tile(t)
{

}

pair<const uint8_t&, const ColorPalette&> GPU::SpriteData::palette() const
{
    if (m_gpu.m_mmc.isCGB()) {
        uint8_t index = this->flags & PALETTE_NUMBER_CGB;

        const auto & [palettes, colors] = m_gpu.m_palettes.sprite;
        return { palettes.at(index), colors.at(index) };
    } else {
        const uint8_t & palette =
            ((this->flags & PALETTE_NUMBER_DMR) ? *mono.at(1) : *mono.at(0));
        return { palette, DMR_PALETTE };
    }
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
        const GB::RGB & color = this->colors.at(i);
        if (ALPHA_TRANSPARENT == color.alpha) { continue; }

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
            const GB::RGB & bg = DMR_PALETTE[dPalette & 0x03].second;
            if (!(display[index] == bg)) {
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
