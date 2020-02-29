#ifndef _CARTRIDGE_H
#define _CARTRIDGE_H

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include "memmap.h"

class Cartridge {
public:
    explicit Cartridge(const std::string & path);
    ~Cartridge() = default;

    inline bool isValid() const { return m_valid; }

    void write(uint16_t address, uint8_t value);
    uint8_t & read(uint16_t address);

    inline bool isCGB() const { return m_cgb; }

private:
    static const uint16_t NINTENDO_LOGO_OFFSET;

    static const uint16_t ROM_HEADER_LENGTH;
    static const uint16_t ROM_NAME_OFFSET;
    static const uint16_t ROM_TYPE_OFFSET;
    static const uint16_t ROM_SIZE_OFFSET;
    static const uint16_t ROM_CGB_OFFSET;
    static const uint16_t ROM_RAM_SIZE_OFFSET;
    static const uint8_t ROM_NAME_MAX_LENGTH;

    static const std::vector<uint8_t> NINTENDO_LOGO;

    static const std::vector<uint16_t> RAM_SIZES;

    enum BankType {
        MBC_NONE = 0x00,
        MBC_1    = 0x01,
        MBC_1R   = 0x02,
        MBC_1RB  = 0x03,
        MBC_2    = 0x05,
        MBC_3TRB = 0x10,
        MBC_3RB  = 0x13,
    };
    enum BankMode { MBC_ROM, MBC_RAM };

    std::string m_path;

    bool m_valid;

    struct {
        std::string name;
        uint32_t size;

        BankType type;
    } m_info;

    class MemoryBank {
    public:
        MemoryBank(Cartridge & cartridge, const std::string & name, uint16_t size, bool battery);
        virtual ~MemoryBank();

        virtual void writeROM(uint16_t address, uint8_t value) = 0;

        void writeRAM(uint16_t address, uint8_t value);

        uint8_t & readROM(uint16_t address);
        uint8_t & readRAM(uint16_t address);

        inline std::string name() const { return m_name; }

        inline size_t size() const { return m_ram.size(); }

    protected:
        Cartridge & m_cartridge;

        std::string m_name;

        std::vector<uint8_t> m_ram;

        bool m_ramEnable;

        uint8_t m_romBank;
        uint8_t m_ramBank;

        bool m_hasBattery;

        std::ofstream m_nvRam;
    };

    class MBC1 : public MemoryBank {
    public:
        MBC1(Cartridge & cartridge, uint16_t size, bool battery);
        ~MBC1() = default;

        void writeROM(uint16_t address, uint8_t value) override;

    private:
        Cartridge::BankMode m_mode;
    };

    class MBC2 : public MemoryBank {
    public:
        MBC2(Cartridge & cartridge, uint16_t size);
        ~MBC2() = default;

        void writeROM(uint16_t address, uint8_t value) override;
    };

    class MBC3 : public MemoryBank {
    public:
        MBC3(Cartridge & cartridge, uint16_t size, bool battery, bool rtc);
        ~MBC3() = default;

        void writeROM(uint16_t address, uint8_t value) override;

    private:
        bool m_rtc;
    };

    std::vector<uint8_t> m_memory;
    std::vector<uint8_t> m_ram;

    std::unique_ptr<MemoryBank> m_bank;

    bool m_cgb;

    MemoryBank *initMemoryBank(uint8_t type);

    inline std::string game() const { return m_info.name; }
};

#endif
