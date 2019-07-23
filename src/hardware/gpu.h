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

#include "interrupt.h"
#include "memmap.h"

#define GPU_SPRITE_COUNT 40

class MemoryController;

namespace GB { struct RGB; };

typedef std::vector<std::shared_ptr<GB::RGB>> ColorArray;
typedef std::vector<uint8_t> Tile;
typedef std::array<GB::RGB, 4> BWPalette;

class GPU {
public:
    explicit GPU(MemoryController & memory);
    ~GPU() = default;

    void cycle();
    void reset();

    ColorArray getColorMap();

    inline uint8_t scanline() const { return m_scanline; }
    
    inline void enableCGB()  { m_cgb = true;  }
    inline void disableCGB() { m_cgb = false; }

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

    static const uint16_t SCREEN_ROWS;
    static const uint16_t SCREEN_COLUMNS;

    static const uint16_t OAM_TICKS;
    static const uint16_t VRAM_TICKS;
    static const uint16_t HBLANK_TICKS;
    static const uint16_t VBLANK_TICKS;

    static const uint16_t PIXELS_PER_ROW;
    static const uint16_t PIXELS_PER_COL;

    static const uint8_t ALPHA_TRANSPARENT;

    struct SpriteData {
        const uint8_t & x;
        const uint8_t & y;

        const uint8_t & flags;

        const uint8_t & palette0;
        const uint8_t & palette1;
        
        uint8_t height;

        uint16_t address;
        uint16_t pointer;

        ColorArray colors;

        SpriteData(
            const uint8_t & col,
            const uint8_t & row,
            const uint8_t & atts,
            const uint8_t & pal0,
            const uint8_t & pal1);

        ~SpriteData() = default;
        
        std::string toString() const;
        bool isVisible() const;
        void render(ColorArray & display, uint8_t dPalette) const;

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
        WINDOW_TILE_SET   = 0x40,
        DISPLAY_ENABLE    = 0x80,
    };

    inline bool isBackgroundEnabled() const { return (m_control & BACKGROUND_ENABLE); }
    inline bool isSpriteEnabled()     const { return (m_control & SPRITE_ENABLE);     }
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
    uint8_t & m_scanline;
    
    RenderState m_state;
    
    uint32_t m_ticks;
    
    bool m_cgb;

    std::array<std::shared_ptr<SpriteData>, GPU_SPRITE_COUNT> m_sprites;
    
    ColorArray lookup(TileMapIndex mIndex, TileSetIndex sIndex);
    Tile lookup(uint16_t address);
    Tile lookup(TileMapIndex mIndex, TileSetIndex sIndex, uint16_t x, uint16_t y);

    ColorArray toRGB(const uint8_t & pal, const Tile & tile, bool white) const;
    ColorArray constrain(const std::vector<ColorArray> & display) const;
    
    std::shared_ptr<GB::RGB> palette(const uint8_t & pal, uint8_t pixel, bool white) const;

    void handleHBlank();
    void handleVBlank();
    void handleOAM();
    void handleVRAM();
    
    RenderState next();

    inline void updateRenderStateStatus(RenderState state)
        { m_status = ((m_status & 0xFC) | uint8_t(state)); }

    void drawSprites(ColorArray & display);
    void readSprite(SpriteData & data);
};

#endif /* SRC_GPU_H_ */
