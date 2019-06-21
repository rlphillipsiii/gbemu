/*
 * memorycontroller.h
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#ifndef MEMORY_CONTROLLER_H_
#define MEMORY_CONTROLLER_H_

#include <cstdint>
#include <array>
#include <vector>
#include <list>
#include <string>
#include <algorithm>

class GPU;

class MemoryController {
public:
    MemoryController();
    ~MemoryController() = default;

    void write(uint16_t address, uint8_t value);
    void writeWord(uint16_t address, uint16_t value);
    uint8_t & read(uint16_t address);
    uint16_t readWord(uint16_t address);

    void reset();

    void unlockBiosRegion();

    void saveBIOS(const std::string & filename);

private:
    static uint8_t DUMMY;
    static const std::vector<uint8_t> BIOS_REGION;

    ////////////////////////////////////////////////////////////////////////////////
    class Region {
    public:
        Region(uint16_t size, uint16_t offset);
        virtual ~Region() = default;

        Region & operator=(const Region &) = delete;
        Region(const Region &) = delete;

        virtual bool isAddressed(uint16_t address) const;

        virtual void write(uint16_t address, uint8_t value);
        virtual uint8_t & read(uint16_t address);

        virtual uint16_t size() const { return uint16_t(m_memory.size()); }

        virtual void reset() { std::fill(m_memory.begin(), m_memory.end(), 0); }

    protected:
        uint16_t m_offset;

        std::vector<uint8_t> m_memory;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class MemoryMappedIO : public Region {
    public:
        MemoryMappedIO(uint16_t address, uint16_t offset);

        void write(uint16_t address, uint8_t value) override;
        uint8_t & read(uint16_t address) override;

        void reset() override;
        
        inline void writeBytes(uint8_t & reg, uint8_t value, uint8_t mask)
            { reg = ((reg & ~mask) | (value & mask)); }

    private:
        uint8_t m_interruptFlags;
        uint8_t m_interruptMask;

        uint8_t m_gpuControl;
        uint8_t m_gpuStatus;
        uint8_t m_gpuScrollX;
        uint8_t m_gpuScrollY;
        uint8_t m_gpuScanline;
        uint8_t m_gpuPalette;
        uint8_t m_gpuOBP1;
        uint8_t m_gpuOBP2;

        uint8_t m_soundEnable;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class WorkingRam : public Region {
    public:
        WorkingRam(uint16_t size, uint16_t offset);

        void write(uint16_t address, uint8_t value) override;
        uint8_t & read(uint16_t address) override;

        bool isAddressed(uint16_t address) const override;

    private:
        uint16_t m_shadowOffset;

        std::vector<uint8_t> m_shadow;

        bool isShadowAddressed(uint16_t address) const;
    };
    ////////////////////////////////////////////////////////////////////////////////

    Region m_bios;
    Region m_rom_0;
    Region m_rom_1;
    Region m_gram;
    Region m_ext;
    WorkingRam m_working;
    MemoryMappedIO m_io;
    Region m_zero;

    std::list<Region*> m_memory;
};

#endif /* SRC_MEMORYCONTROLLER_H_ */
