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
#include <memory>

#include "cartridge.h"

class MemoryController {
public:
    MemoryController();
    ~MemoryController() = default;

    void initialize(uint16_t address, uint8_t value);

    void write(uint16_t address, uint8_t value);
    uint8_t & read(uint16_t address);

    void reset();
    void setCartridge(const std::string & filename);

    bool isCartridgeValid() const;

    void saveBIOS(const std::string & filename);

    inline bool isRtcResetRequested() const
        { return m_io.isRtcResetRequested(); }

    inline void clearRtcReset()    { m_io.clearRtcReset(); }
    inline void unlockBiosRegion() { m_memory.pop_front(); }

    inline bool inBios() const { return (m_memory.front() == &m_bios); }
    
private:
    static uint8_t DUMMY;
    static const uint16_t MBC_TYPE_ADDRESS;
    
    static const std::vector<uint8_t> BIOS_REGION;

    enum BankType { MBC_NONE, MBC1, MBC2, MBC3 };
    enum BankMode { MBC_ROM, MBC_RAM };
    
    ////////////////////////////////////////////////////////////////////////////////
    class Region {
    public:
        explicit Region(uint16_t size, uint16_t offset);
        virtual ~Region() = default;

        Region & operator=(const Region &) = delete;
        Region(const Region &) = delete;

        virtual bool isAddressed(uint16_t address) const;
        virtual inline bool isWritable() const { return true; }
        
        virtual void write(uint16_t address, uint8_t value);
        virtual uint8_t & read(uint16_t address);

        virtual inline uint16_t size() const { return uint16_t(m_memory.size()); }

        virtual void reset() { std::fill(m_memory.begin(), m_memory.end(), resetValue()); }

        inline void enableInit()  { m_initializing = true;  }
        inline void disableInit() { m_initializing = false; }

        virtual inline uint8_t resetValue() const { return 0x00; }
        
    protected:
        uint16_t m_offset;
        uint16_t m_initializing;

        std::vector<uint8_t> m_memory;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class MemoryMappedIO : public Region {
    public:
        explicit MemoryMappedIO(MemoryController & parent, uint16_t address, uint16_t offset);

        void write(uint16_t address, uint8_t value) override;

        void reset() override;

        inline void writeBytes(uint16_t ptr, uint8_t value, uint8_t mask)
            { writeBytes(Region::read(ptr), value, mask); }
        inline void writeBytes(uint8_t & reg, uint8_t value, uint8_t mask)
            { reg = ((reg & ~mask) | (value & mask)); }

        inline uint8_t resetValue() const override { return 0xFF; }

        inline bool isRtcResetRequested() const { return m_rtcReset; }

        inline void clearRtcReset() { m_rtcReset = false; }

    private:
        MemoryController & m_parent;

        bool m_rtcReset;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class WorkingRam : public Region {
    public:
        explicit WorkingRam(uint16_t size, uint16_t offset);

        void write(uint16_t address, uint8_t value) override;
        uint8_t & read(uint16_t address) override;

        bool isAddressed(uint16_t address) const override;

    private:
        uint16_t m_shadowOffset;

        std::vector<uint8_t> m_shadow;

        bool isShadowAddressed(uint16_t address) const;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class ReadOnly : public Region {
    public:
        explicit ReadOnly(uint16_t size, uint16_t offset);

        inline bool isWritable() const override { return false; }

        void write(uint16_t address, uint8_t value) override;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class Unusable : public Region {
    public:
        explicit Unusable(uint16_t size, uint16_t offset)
            : Region(size, offset), m_dummy(0xFF) { m_memory.resize(0); }
        ~Unusable() = default;

        void write(uint16_t, uint8_t) override { }
        uint8_t & read(uint16_t) override { return m_dummy; }

    private:
        uint8_t m_dummy;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ////////////////////////////////////////////////////////////////////////////////
    class Removable : public Region {
    public:
        explicit Removable() : Region(0, 0) { }
        ~Removable() = default;

        void write(uint16_t address, uint8_t value) override;
        uint8_t & read(uint16_t address) override;

        bool isAddressed(uint16_t address) const override;

        void load(const std::string & filename);

        inline void reset() override { }
        inline bool isValid() const { return m_cartridge->isValid(); }
        
    private:
        static uint8_t EMPTY;
        
        std::shared_ptr<Cartridge> m_cartridge;
    };
    ////////////////////////////////////////////////////////////////////////////////

    ReadOnly m_bios;
    Removable m_cartridge;
    Region m_gram;
    WorkingRam m_working;
    Region m_graphics;
    MemoryMappedIO m_io;
    Region m_zero;
    Unusable m_unusable;

    struct {
        BankType type;
        bool ramEn;
        uint8_t bank;
        BankMode mode;
    } m_mbc;
    
    std::list<Region*> m_memory;
    
    Region *find(uint16_t address) const;

    void initMemoryBank();
};

#endif /* SRC_MEMORYCONTROLLER_H_ */
