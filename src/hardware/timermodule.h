/*
 * timermodule.h
 *
 *  Created on: Jun 24, 2019
 *      Author: Robert Phillips III
 */

#ifndef TIMERMODULE_H_
#define TIMERMODULE_H_

#define TIMEOUT_SEL_COUNT 4

#include <cstdint>
#include <array>

class MemoryController;

using TimeoutMapArray = std::array<uint16_t, TIMEOUT_SEL_COUNT>;

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
#ifdef UNIT_TEST
    friend class TimerTest;
#endif
    static const uint16_t RTC_INCREMENT;

    static const uint16_t TIMEOUT_4K;
    static const uint16_t TIMEOUT_252K;
    static const uint16_t TIMEOUT_65K;
    static const uint16_t TIMEOUT_16K;

    static const TimeoutMapArray TIMEOUT_MAP;

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
