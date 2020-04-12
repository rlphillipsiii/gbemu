/*
 * memorycontroller.h
 *
 *  Created on: May 25, 2019
 *      Author: Robert Phillips III
 */

#ifndef MEMORY_CONTROLLER_H_
#define MEMORY_CONTROLLER_H_

#include <cstdint>
#include <vector>
#include <string>

class MemoryController final {
public:
    MemoryController() : m_memory(MEM_SIZE) { }
    ~MemoryController() = default;

    MemoryController(const MemoryController &) = delete;
    MemoryController(MemoryController &&) = delete;
    MemoryController(const MemoryController &&) = delete;
    MemoryController & operator=(const MemoryController &) = delete;

    void initialize(uint16_t, uint8_t) { }

    void write(uint16_t address, uint8_t value);
    uint8_t & read(uint16_t address);
    const uint8_t & peek(uint16_t address);

    void reset() { }
    void setCartridge(const std::string&) { }

    void saveBIOS(const std::string&) { }

    inline bool isRtcResetRequested() const { return false; }

    inline void clearRtcReset()    { }
    inline void unlockBiosRegion() { }

    inline bool inBios() const { return false; }
    inline bool isCartridgeValid() const { return false; }

    inline bool isCGB() const { return true; }

    inline uint8_t romBank() const { return 0; }
    inline uint8_t ramBank() const { return 0; }

private:
    static constexpr uint16_t MEM_SIZE = 0xFFFF;

    std::vector<uint8_t> m_memory;
};

#endif /* SRC_MEMORYCONTROLLER_H_ */
