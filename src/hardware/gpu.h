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

class MemoryController;

typedef std::vector<uint8_t> Tile;

class Processor;

class GPU {
public:
    struct RGB {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;
    };

    enum MapIndex {
        MAP_0 = 0,
        MAP_1 = 1,
    };

    GPU(MemoryController & memory);
    ~GPU() = default;

    void cycle();
    void reset();

    std::vector<RGB> lookup(MapIndex index);
    
    inline void setCPU(Processor & cpu) { m_cpu = &cpu; }
    
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
    
    MemoryController & m_memory;

    uint8_t & m_control;
    uint8_t & m_status;
    uint8_t m_pallete;
    uint8_t & m_x;
    uint8_t & m_y;
    uint8_t & m_scanline;
    
    RenderState m_state;
    
    uint32_t m_ticks;

    Processor *m_cpu;
    
    Tile lookup(uint16_t address);
    Tile lookup(MapIndex index, uint16_t x, uint16_t y);

    std::vector<RGB> toRGB(const Tile & tile);
    std::vector<RGB> constrain(const std::vector<std::vector<RGB>> & display);
    
    void handleHBlank();
    void handleVBlank();
    void handleOAM();
    void handleVRAM();
    
    RenderState next();

    inline void updateRenderStateStatus(RenderState state)
        { m_status &= 0xFC; m_status |= uint8_t(state); }
};

#endif /* SRC_GPU_H_ */
