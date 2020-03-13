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
    enum ClockSpeed { SPEED_NORMAL = 0x00, SPEED_DOUBLE = 0x01, };

    explicit TimerModule(MemoryController & memory);
    ~TimerModule() = default;

    void reset();

    void cycle(uint8_t ticks);

    void setSpeed(ClockSpeed speed) { m_speed = speed; }

    inline ClockSpeed getSpeed() const { return m_speed; }

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

    enum ControlMask { TIMER_FREQUENCY = 0x03, TIMER_ENABLE = 0x04, };

    MemoryController & m_memory;

    uint8_t & m_divider;
    uint8_t & m_counter;
    uint8_t & m_modulo;
    uint8_t & m_control;

    uint16_t m_ticks;
    uint16_t m_rtc;

    ClockSpeed m_speed;

    uint16_t m_timeout;
};

#endif /* TIMERMODULE_H_ */
