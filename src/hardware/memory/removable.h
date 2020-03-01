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

    bool isAddressed(uint16_t address) const override;

    void load(const std::string & filename);
    bool isValid() const;

    inline void reset() override { }

private:
    static uint8_t EMPTY;

    std::unique_ptr<Cartridge> m_cartridge;
};

#endif
