#ifndef _CARTRIDGE_H
#define _CARTRIDGE_H

#include <cstdint>
#include <string>
#include <vector>

#include "memmap.h"

class Cartridge {
public:
    explicit Cartridge(const std::string & path);
    ~Cartridge() = default;

    inline uint8_t & operator[](int index)
        { return (size_t(index) < m_memory.size()) ? m_memory[index] : m_empty; }

    inline uint8_t at(int index) const
        { return (size_t(index) < m_memory.size()) ? m_memory.at(index) : m_empty; }

    inline uint8_t & readBank(uint16_t offset)
        { return m_memory.at(offset + (ROM_0_SIZE * m_info.mbc.bank)); }

    inline bool isValid() const { return m_valid; }

    inline uint8_t bank() const { return m_info.mbc.bank; }

    void write(uint16_t address, uint8_t value);
    
private:
    static const uint16_t NINTENDO_LOGO_OFFSET;
    
    static const uint16_t ROM_HEADER_LENGTH;
    static const uint16_t ROM_NAME_OFFSET;
    static const uint16_t ROM_TYPE_OFFSET;
    static const uint16_t ROM_SIZE_OFFSET;
    static const uint16_t ROM_RAM_SIZE_OFFSET;
    static const uint8_t ROM_NAME_MAX_LENGTH;

    static const std::vector<uint8_t> NINTENDO_LOGO;
    
    enum BankType {
        MBC_NONE = 0x00,
        MBC_1    = 0x01,
        MBC_1R   = 0x02,
        MBC_1RB  = 0x03,
        MBC_2    = 0x05,
    };
    enum BankMode { MBC_ROM, MBC_RAM };
    
    std::string m_path;

    bool m_valid;

    uint8_t m_empty;

    struct {
        std::string name;
        uint32_t size;

        struct {
            BankType type;
            bool ramEn;
            uint8_t bank;
            BankMode mode;
        } mbc;
    } m_info;
    
    std::vector<uint8_t> m_memory;

    void writeROM(uint16_t address, uint8_t value);
    void writeRAM(uint16_t address, uint8_t value);
};

#endif
