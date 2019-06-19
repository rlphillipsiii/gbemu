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

#include "interrupt.h"
#include "memmap.h"

class MemoryController;

typedef std::vector<uint8_t> Tile;

class GPU {
public:
    struct RGB { uint8_t red; uint8_t green; uint8_t blue; uint8_t alpha; };

    enum MapIndex {
        MAP_0 = 0,
        MAP_1 = 1,
    };

    GPU(MemoryController & memory);
    ~GPU() = default;

    void cycle();
    void reset();

    std::vector<RGB> getColorMap();

    inline uint8_t scanline() const { return m_scanline; }
    
private:
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

    static const uint16_t SCREEN_ROWS;
    static const uint16_t SCREEN_COLUMNS;

    static const uint16_t OAM_TICKS;
    static const uint16_t VRAM_TICKS;
    static const uint16_t HBLANK_TICKS;
    static const uint16_t VBLANK_TICKS;

    static const uint16_t PIXELS_PER_ROW;
    static const uint16_t PIXELS_PER_COL;

    enum RenderState {
        HBLANK = 0,
        VBLANK = 1,
        OAM    = 2,
        VRAM   = 3,
    };

    enum ControlBitMask {
        BACKGROUND_ENABLE = 0x00,
        SPRITE_ENABLE     = 0x01,
        SPRITE_SIZE       = 0x02,
        BACKGROUND_MAP    = 0x04,
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
        COINCIDENCE_FLAG      = 0x02,
        HBLANK_INTERRUPT      = 0x04,
        VBLANK_INTERRUPT      = 0x10,
        OAM_INTERRUPT         = 0x20,
        COINCIDENCE_INTERRUPT = 0x40,
    };

    inline bool isHBlankInterruptEnabled()      const { return (m_status & HBLANK_INTERRUPT);      }
    inline bool isVBlankInterruptEnabled()      const { return (m_status & VBLANK_INTERRUPT);      }
    inline bool isOAMInterruptEnabled()         const { return (m_status & OAM_INTERRUPT);         }
    inline bool isCoincidenceInterruptEnabled() const { return (m_status & COINCIDENCE_INTERRUPT); }

    MemoryController & m_memory;

    uint8_t & m_control;
    uint8_t & m_status;
    uint8_t m_pallete;
    uint8_t & m_x;
    uint8_t & m_y;
    uint8_t & m_scanline;
    
    RenderState m_state;
    
    uint32_t m_ticks;
    
    std::vector<RGB> lookup(MapIndex index);
    Tile lookup(uint16_t address);
    Tile lookup(MapIndex index, uint16_t x, uint16_t y);

    std::vector<RGB> toRGB(const Tile & tile) const;
    std::vector<RGB> constrain(const std::vector<std::vector<RGB>> & display) const;
    
    RGB pallette(uint8_t pixel) const;

    void handleHBlank();
    void handleVBlank();
    void handleOAM();
    void handleVRAM();
    
    RenderState next();

    inline void updateRenderStateStatus(RenderState state)
        { m_status = ((m_status & 0xFC) | uint8_t(state)); }

    void setInterrupt(InterruptMask interrupt);
};

#endif /* SRC_GPU_H_ */
