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

class MemoryController;

struct TimerModule {
public:
    enum ControlMask {
        TIMER_FREQUENCY = 0x03,
        TIMER_ENABLE    = 0x04,
    };

    uint8_t & m_divider;
    uint8_t & m_counter;
    uint8_t & m_modulo;
    uint8_t & m_control;

    explicit TimerModule(MemoryController & memory);
    ~TimerModule() = default;

    void reset();

    void cycle();

private:
    MemoryController & m_memory;
};

#endif /* TIMERMODULE_H_ */
