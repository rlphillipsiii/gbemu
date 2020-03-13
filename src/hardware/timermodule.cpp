/*
 * timermodule.cpp
 *
 *  Created on: Jun 24, 2019
 *      Author: Robert Phillips III
 */
#include <cassert>

#include "timermodule.h"
#include "memmap.h"
#include "interrupt.h"

const uint16_t TimerModule::RTC_INCREMENT = 512;

const uint16_t TimerModule::TIMEOUT_4K   = 1024;
const uint16_t TimerModule::TIMEOUT_252K = 16;
const uint16_t TimerModule::TIMEOUT_65K  = 64;
const uint16_t TimerModule::TIMEOUT_16K  = 256;

const TimeoutMapArray TimerModule::TIMEOUT_MAP = {
    { TIMEOUT_4K, TIMEOUT_252K, TIMEOUT_65K, TIMEOUT_16K, },
};

TimerModule::TimerModule(MemoryController & memory)
    : m_memory(memory),
      m_divider(memory.read(CPU_TIMER_DIV_ADDRESS)),
      m_counter(memory.read(CPU_TIMER_COUNTER_ADDRESS)),
      m_modulo(memory.read(CPU_TIMER_MODULO_ADDRESS)),
      m_control(memory.read(CPU_TIMER_CONTROL_ADDRESS))
{
    reset();
}

void TimerModule::reset()
{
    m_ticks = m_rtc = 0;

    m_divider = m_counter = m_modulo = m_control = 0x00;
}

void TimerModule::cycle(uint8_t ticks)
{
    // Check to see if the memory module has a pending request for a reset of
    // the real time clock.  If so, we need to reset the rtc counter and then
    // clear the request.
    if (m_memory.isRtcResetRequested()) {
        m_divider = m_rtc = 0x00;

        m_memory.clearRtcReset();
    }

    m_rtc += ticks;

    // Check to see if the number of ticks corresponding to 1 increment of
    // the RTC has elapsed and increment it if so.
    while (m_rtc >= RTC_INCREMENT) {
        m_divider++;

        m_rtc -= RTC_INCREMENT;
    }

    if (!(m_control & TIMER_ENABLE)) { return; }

    m_ticks += ticks;

    // Read the frequency bits in the control register and look up how many
    // clock cycles need to elapse until we need to increment the counter
    // register.
    m_timeout = TIMEOUT_MAP.at(m_control & TIMER_FREQUENCY);

    while (m_ticks >= m_timeout) {
        // We've got the correct number of ticks to increment the actual
        // counter, so increment it now.  If the register is set to 0xFF,
        // then this increment is going to overflow the register, so we
        // need to set the timer interrupt bit and set the counter back to
        // its initial value.
        if (0xFF == m_counter++) {
            Interrupts::set(m_memory, InterruptMask::TIMER);

            m_counter = m_modulo;
        }

        m_ticks -= m_timeout;
    }
}
