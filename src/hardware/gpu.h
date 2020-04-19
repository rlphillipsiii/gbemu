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
#include <array>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <cassert>

#include "interrupt.h"
#include "memmap.h"
#include "memoryregion.h"
#include "gbrgb.h"

#define GPU_SPRITE_COUNT 40
#define GPU_TILES_PER_SET 256
#define GPU_TILE_SET_COUNT 2
#define GPU_COLORS_PER_PALETTE 4
#define GPU_CGB_PALETTE_COUNT 8
#define GPU_BANK_COUNT 2

class MemoryController;

using Tile     = std::vector<const uint8_t*>;
using TileSet  = std::array<Tile, GPU_TILES_PER_SET>;
using TileBank = std::array<TileSet, GPU_TILE_SET_COUNT>;

using ColorPixel   = std::pair<std::array<uint8_t, 2>, GB::RGB>;
using ColorPalette = std::array<ColorPixel, GPU_COLORS_PER_PALETTE>;
using CgbColors    = std::array<ColorPalette, GPU_CGB_PALETTE_COUNT>;

class GPU : public MemoryRegion {
public:
    explicit GPU(MemoryController & memory);
    ~GPU() = default;

    void cycle(uint8_t ticks);
    void reset() override;

    inline uint8_t scanline() const { return m_scanline; }

    inline ColorArray && getColorMap()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        return std::move(m_screen);
    }

    void write(uint16_t address, uint8_t value) override;
    uint8_t & read(uint16_t address) override;

    void writeBgPalette(uint8_t index, uint8_t value);
    void writeSpritePalette(uint8_t index, uint8_t value);

    uint8_t & readBgPalette(uint8_t index);
    uint8_t & readSpritePalette(uint8_t index);

    void dma(uint8_t value);

private:
    static const ColorPalette DMG_PALETTE;

    static const uint8_t BANK_COUNT;

    static const uint16_t TILE_SIZE;
    static const uint16_t TILES_PER_SET;

    static constexpr uint16_t TILE_PIXELS_PER_ROW = 8;
    static constexpr uint16_t TILE_PIXELS_PER_COL = 8;

    static const uint16_t TILE_SET_0_OFFSET;
    static const uint16_t TILE_SET_1_OFFSET;

    static const uint16_t TILE_MAP_ROWS;
    static const uint16_t TILE_MAP_COLUMNS;

    static const uint16_t TILE_MAP_0_OFFSET;
    static const uint16_t TILE_MAP_1_OFFSET;

    static const uint16_t BG_ATTRIBUTES_TABLE;

    static const uint16_t SPRITE_ATTRIBUTES_TABLE;
    static const uint8_t SPRITE_COUNT;
    static const uint8_t SPRITE_BYTES_PER_ATTRIBUTE;
    static const uint8_t SPRITE_WIDTH;
    static const uint8_t SPRITE_HEIGHT_NORMAL;
    static const uint8_t SPRITE_HEIGHT_EXTENDED;
    static const uint8_t SPRITE_X_OFFSET;
    static const uint8_t SPRITE_Y_OFFSET;

    static const uint16_t SCREEN_ROWS;
    static const uint16_t SCREEN_COLUMNS;

    static const uint16_t OAM_TICKS;
    static const uint16_t VRAM_TICKS;
    static const uint16_t HBLANK_TICKS;
    static const uint16_t VBLANK_TICKS;

    static const uint16_t SCANLINE_MAX;
    static const uint16_t SCANLINE_TICKS;

    static const uint16_t PIXELS_PER_ROW;
    static const uint16_t PIXELS_PER_COL;

    static const uint8_t ALPHA_TRANSPARENT;

    static const uint8_t WINDOW_ROW_OFFSET;

    static constexpr uint16_t DMA_CHUNK_SIZE = 16;

    using TileRow = std::array<GB::RGB, TILE_PIXELS_PER_ROW>;

    struct SpriteData {
        GPU & m_gpu;

        const uint8_t & x;
        const uint8_t & y;

        const uint8_t & flags;

        const uint8_t & tile;

        uint8_t height;
        uint16_t address;

        TileRow colors;

        std::array<const uint8_t*, 2> mono;

        SpriteData(
            GPU & gpu,
            const uint8_t & col,
            const uint8_t & row,
            const uint8_t & tile,
            const uint8_t & atts);
        ~SpriteData() = default;

        std::string toString() const;
        bool isVisible() const;
        void render(ColorArray & display, ColorArray & bg);

        std::pair<std::optional<uint8_t>, const ColorPalette&> palette() const;
    };

    enum MemoryBank { BANK_0 = 0, BANK_1 = 1 };

    enum TileMapIndex { TILEMAP_0 = 0, TILEMAP_1 = 1, };
    enum TileSetIndex { TILESET_0 = 0, TILESET_1 = 1, };

    enum RenderState { HBLANK = 0, VBLANK = 1, OAM = 2, VRAM = 3, };

    enum ControlBitMask {
        BACKGROUND_ENABLE = 0x01,
        SPRITE_ENABLE     = 0x02,
        SPRITE_SIZE       = 0x04,
        BACKGROUND_MAP    = 0x08,
        TILE_SET_SELECT   = 0x10,
        WINDOW_ENABLE     = 0x20,
        WINDOW_MAP        = 0x40,
        DISPLAY_ENABLE    = 0x80,
    };

    inline bool isBackgroundEnabled() const { return (m_control & BACKGROUND_ENABLE); }
    inline bool areSpritesEnabled()   const { return (m_control & SPRITE_ENABLE);     }
    inline bool isWindowEnabled()     const { return (m_control & WINDOW_ENABLE);     }
    inline bool isDisplayEnabled()    const { return (m_control & DISPLAY_ENABLE);    }

    enum StatusBitMask {
        RENDER_MODE           = 0x03,
        COINCIDENCE_FLAG      = 0x04,
        HBLANK_INTERRUPT      = 0x08,
        VBLANK_INTERRUPT      = 0x10,
        OAM_INTERRUPT         = 0x20,
        COINCIDENCE_INTERRUPT = 0x40,
    };

    enum SpriteAttributesMask {
        PALETTE_NUMBER_CGB = 0x07,
        TILE_BANK_CGB      = 0x08,
        PALETTE_NUMBER_DMG = 0x10,
        FLIP_X             = 0x20,
        FLIP_Y             = 0x40,
        OBJECT_PRIORITY    = 0x80,
    };

    enum BackgroundAttributesMask {
        BG_PALETTE_NUMBER = 0x07,
        BG_TILE_BANK      = 0x08,
        BG_FLIP_X         = 0x20,
        BG_FLIP_Y         = 0x40,
        BG_OAM_PRIORITY   = 0x80,
    };

    inline bool isHBlankInterruptEnabled() const
        { return (m_status & HBLANK_INTERRUPT); }
    inline bool isVBlankInterruptEnabled() const
        { return (m_status & VBLANK_INTERRUPT); }
    inline bool isOAMInterruptEnabled() const
        { return (m_status & OAM_INTERRUPT); }
    inline bool isCoincidenceInterruptEnabled() const
        { return (m_status & COINCIDENCE_INTERRUPT); }

    MemoryController & m_mmc;

    uint8_t & m_control;
    uint8_t & m_status;
    uint8_t & m_palette;
    uint8_t & m_x;
    uint8_t & m_y;
    uint8_t & m_winX;
    uint8_t & m_winY;
    uint8_t & m_scanline;
    uint8_t & m_lyc;

    RenderState m_state;

    uint32_t m_ticks;
    uint16_t m_vscan;

    std::mutex m_lock;

    ColorArray m_screen;
    ColorArray m_buffer;
    ColorArray m_bg;

    struct {
        uint16_t length;

        uint16_t source;
        uint16_t destination;
    } m_dma;

    std::array<TileBank, GPU_BANK_COUNT> m_tiles;
    std::array<std::shared_ptr<SpriteData>, GPU_SPRITE_COUNT> m_sprites;

    struct {
        CgbColors bg;
        CgbColors sprite;
    } m_palettes;

    void draw(TileSetIndex set, TileMapIndex background, TileMapIndex window);

    std::pair<const uint8_t&, const Tile&>
        lookup(TileMapIndex mIndex, TileSetIndex sIndex, uint16_t x, uint16_t y);

    void toRGB(
        TileRow & colors,
        const ColorPalette & rgb,
        std::optional<uint8_t> pal,
        const Tile & tile,
        uint8_t row,
        bool white,
        bool flip) const;

    void handleHBlank();
    void handleVBlank(uint8_t ticks);
    void handleOAM();
    void handleVRAM();

    RenderState next();

    void write(MemoryBank bank, uint16_t index, uint8_t value);
    uint8_t & read(MemoryBank bank, uint16_t index);

    inline const Tile & getTile(MemoryBank bank, TileSetIndex index, uint16_t tile) const
    {
        assert(size_t(bank) < m_tiles.size());
        assert(size_t(index) < m_tiles.at(bank).size());
        assert(size_t(tile) < m_tiles.at(bank).at(index).size());
        return m_tiles.at(bank).at(index).at(tile);
    }

    inline void updateRenderStateStatus(RenderState state)
        { m_status = ((m_status & 0xFC) | uint8_t(state)); }

    void updateScreen();
    void incrementScanline();

    bool isWindowSelected(uint8_t x, uint8_t y);

    void drawSprites(ColorArray & display, ColorArray & bg);
    void drawBackground(TileSetIndex set, TileMapIndex background, TileMapIndex window);

    void readSprite(SpriteData & data);

    void writePalette(CgbColors & colors, uint8_t index, uint8_t value);
    uint8_t & readPalette(CgbColors & colors, uint8_t index);

    void initSpriteCache();
    void initTileCache();
};

#endif /* SRC_GPU_H_ */
