#ifndef _REMOVABLE_H
#define _REMOVABLE_H

#include <cstdint>
#include <memory>
#include <string>

#include "memoryregion.h"
#include "cartridge.h"

class MemoryController;

class Removable : public MemoryRegion {
public:
    explicit Removable(MemoryController & parent)
        : MemoryRegion(parent, 0, 0) { }
    ~Removable() = default;

    void write(uint16_t address, uint8_t value) override;
    uint8_t & read(uint16_t address) override;

    inline bool isReadOnly() const override { return true; }

    bool isAddressed(uint16_t address) const override;

    void load(const std::string & filename);
    bool isValid() const;

    inline void reset() override { }

    inline bool isCGB() const
        { return (m_cartridge) ? m_cartridge->isCGB() : false; }
    inline bool check() const
        { return (m_cartridge) ? m_cartridge->check() : false; }

    inline uint8_t romBank() const
        { return (m_cartridge) ? m_cartridge->romBank() : 0; }
    inline uint8_t ramBank() const
        { return (m_cartridge) ? m_cartridge->ramBank() : 0; }

private:
    static uint8_t EMPTY;

    std::unique_ptr<Cartridge> m_cartridge;
};

#endif
