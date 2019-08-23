/*
 * timermodule.h
 *
 *  Created on: Jun 24, 2019
 *      Author: Robert Phillips III
 */

#ifndef TIMERMODULE_H_
#define TIMERMODULE_H_

#include <cstdint>
#include <array>
#include <unordered_map>

class MemoryController;

struct TimerModule {
public:
    enum ControlMask {
        TIMER_FREQUENCY = 0x03,
        TIMER_ENABLE    = 0x04,
    };

    explicit TimerModule(MemoryController & memory);
    ~TimerModule() = default;

    void reset();

    void cycle(uint8_t ticks);

private:
    static const uint16_t RTC_INCREMENT;

    static const uint16_t TIMEOUT_4K;
    static const uint16_t TIMEOUT_252K;
    static const uint16_t TIMEOUT_65K;
    static const uint16_t TIMEOUT_16K;
    
    static const std::unordered_map<uint8_t, uint16_t> TIMEOUT_MAP;

    MemoryController & m_memory;

    uint8_t & m_divider;
    uint8_t & m_counter;
    uint8_t & m_modulo;
    uint8_t & m_control;

    uint16_t m_ticks;
    uint16_t m_rtc;
    
    uint16_t m_timeout;
};

#endif /* TIMERMODULE_H_ */
