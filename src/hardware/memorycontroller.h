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
#include <functional>
#include <optional>

#include "memoryregion.h"
#include "mappedio.h"
#include "readonly.h"
#include "removable.h"
#include "unusable.h"
#include "workingram.h"

class MemoryController {
public:
    MemoryController(GameBoy & parent);
    ~MemoryController() = default;

    MemoryController(const MemoryController &) = delete;
    MemoryController(MemoryController &&) = delete;
    MemoryController(const MemoryController &&) = delete;
    MemoryController & operator=(const MemoryController &) = delete;

    void initialize(uint16_t address, uint8_t value);

    void write(uint16_t address, uint8_t value);
    uint8_t & read(uint16_t address);
    const uint8_t & peek(uint16_t address);

    void reset();
    void setCartridge(const std::string & filename);

    void saveBIOS(const std::string & filename);

    inline bool isRtcResetRequested() const
        { return m_io.isRtcResetRequested(); }

    inline void clearRtcReset()    { m_io.clearRtcReset(); }
    inline void unlockBiosRegion() { if (inBios()) { m_memory.pop_front(); } }

    inline bool inBios() const { return (&m_memory.front().get() == &m_bios); }
    inline bool isCartridgeValid() const { return m_cartridge.isValid(); }

    inline bool isCGB() const { return m_cartridge.isCGB(); }

    inline uint8_t romBank() const { return m_cartridge.romBank(); }
    inline uint8_t ramBank() const { return m_cartridge.ramBank(); }

private:
    static uint8_t DUMMY;
    static const uint16_t MBC_TYPE_ADDRESS;

    static const std::vector<uint8_t> DMG_BIOS_REGION;
    static const std::vector<uint8_t> CGB_BIOS_REGION;

    enum BankType { MBC_NONE, MBC1, MBC2, MBC3 };
    enum BankMode { MBC_ROM, MBC_RAM };

    Bios m_bios;
    Removable m_cartridge;
    WorkingRam m_working;
    MemoryRegion m_oam;
    MappedIO m_io;
    MemoryRegion m_zero;
    Unusable m_unusable;

    struct {
        BankType type;
        bool ramEn;
        uint8_t bank;
        BankMode mode;
    } m_mbc;

    GameBoy & m_parent;

    std::list<std::reference_wrapper<MemoryRegion>> m_memory;

    std::optional<std::reference_wrapper<MemoryRegion>> find(uint16_t address) const;

    void init();
    void initMemoryBank();
};

#endif /* SRC_MEMORYCONTROLLER_H_ */
