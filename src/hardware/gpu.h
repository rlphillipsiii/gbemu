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

#include "interrupt.h"
#include "memmap.h"

#define GPU_SPRITE_COUNT 40

#define GPU_TILES_PER_SET 256

class MemoryController;

namespace GB { struct RGB; };

typedef std::vector<GB::RGB> ColorArray;
typedef std::vector<const uint8_t*> Tile;
typedef std::array<GB::RGB, 4> BWPalette;

class GPU {
public:
    explicit GPU(MemoryController & memory);
    ~GPU() = default;

    void cycle(uint8_t ticks);
    void reset();

    inline uint8_t scanline() const { return m_scanline; }

    inline void enableCGB()  { m_cgb = true;  }
    inline void disableCGB() { m_cgb = false; }

    inline ColorArray && getColorMap()
    {
        std::lock_guard<std::mutex> guard(m_lock);
        return std::move(m_screen);
    }

private:
    static const BWPalette NON_CGB_PALETTE;

    static const uint16_t TILE_SIZE;
    static const uint16_t TILES_PER_SET;

    static const uint16_t TILE_PIXELS_PER_ROW;
    static const uint16_t TILE_PIXELS_PER_COL;

    static const uint16_t TILE_SET_0_OFFSET;
    static const uint16_t TILE_SET_1_OFFSET;

    static const uint16_t TILE_MAP_ROWS;
    static const uint16_t TILE_MAP_COLUMNS;

    static const uint16_t TILE_MAP_0_OFFSET;
    static const uint16_t TILE_MAP_1_OFFSET;

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

    static const uint16_t PIXELS_PER_ROW;
    static const uint16_t PIXELS_PER_COL;

    static const uint8_t ALPHA_TRANSPARENT;

    static const uint8_t WINDOW_ROW_OFFSET;

    struct SpriteData {
        GPU & m_gpu;

        const uint8_t & x;
        const uint8_t & y;

        const uint8_t & flags;

        const uint8_t & palette0;
        const uint8_t & palette1;

        const uint8_t & tile;

        uint8_t height;
        uint16_t address;

        ColorArray colors;

        SpriteData(
            GPU & gpu,
            const uint8_t & col,
            const uint8_t & row,
            const uint8_t & tile,
            const uint8_t & atts,
            const uint8_t & pal0,
            const uint8_t & pal1);

        ~SpriteData() = default;

        std::string toString() const;
        bool isVisible() const;
        void render(ColorArray & display, uint8_t dPalette);

        uint8_t palette() const;
    };

    enum TileMapIndex { TILEMAP_0 = 0, TILEMAP_1 = 1, };
    enum TileSetIndex { TILESET_0 = 0, TILESET_1 = 1, };

    enum RenderState {
        HBLANK = 0,
        VBLANK = 1,
        OAM    = 2,
        VRAM   = 3,
    };

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
        PALETTE_NUMBER_CGB     = 0x07,
        TILE_BANK_CGB          = 0x08,
        PALETTE_NUMBER_NON_CGB = 0x10,
        FLIP_X                 = 0x20,
        FLIP_Y                 = 0x40,
        OBJECT_PRIORITY        = 0x80,
    };

    inline bool isHBlankInterruptEnabled()      const { return (m_status & HBLANK_INTERRUPT);      }
    inline bool isVBlankInterruptEnabled()      const { return (m_status & VBLANK_INTERRUPT);      }
    inline bool isOAMInterruptEnabled()         const { return (m_status & OAM_INTERRUPT);         }
    inline bool isCoincidenceInterruptEnabled() const { return (m_status & COINCIDENCE_INTERRUPT); }

    MemoryController & m_memory;

    uint8_t & m_control;
    uint8_t & m_status;
    uint8_t & m_palette;
    uint8_t & m_sPalette0;
    uint8_t & m_sPalette1;
    uint8_t & m_x;
    uint8_t & m_y;
    uint8_t & m_winX;
    uint8_t & m_winY;
    uint8_t & m_scanline;

    RenderState m_state;

    uint32_t m_ticks;

    bool m_cgb;

    std::mutex m_lock;

    ColorArray m_screen;
    ColorArray m_buffer;

    std::array<std::array<Tile, GPU_TILES_PER_SET>, 2> m_tiles;
    std::array<std::shared_ptr<SpriteData>, GPU_SPRITE_COUNT> m_sprites;

    void draw(TileSetIndex set, TileMapIndex background, TileMapIndex window);

    const Tile & lookup(TileMapIndex mIndex, TileSetIndex sIndex, uint16_t x, uint16_t y);

    ColorArray toRGB(
        const uint8_t & pal,
        const Tile & tile,
        uint8_t row,
        bool white,
        bool flip) const;

    GB::RGB palette(const uint8_t & pal, uint8_t pixel, bool white) const;

    void handleHBlank();
    void handleVBlank();
    void handleOAM();
    void handleVRAM();

    RenderState next();

    inline const Tile & getTile(TileSetIndex index, uint16_t tile) const
        { return m_tiles.at(index).at(tile); }

    inline void updateRenderStateStatus(RenderState state)
        { m_status = ((m_status & 0xFC) | uint8_t(state)); }

    void updateScreen();

    bool isWindowSelected(uint8_t x, uint8_t y);

    void drawSprites(ColorArray & display);
    void drawBackground(TileSetIndex set, TileMapIndex background, TileMapIndex window);

    void readSprite(SpriteData & data);
};

#endif /* SRC_GPU_H_ */
