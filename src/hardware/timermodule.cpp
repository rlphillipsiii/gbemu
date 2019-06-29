/*
 * timermodule.cpp
 *
 *  Created on: Jun 24, 2019
 *      Author: Robert Phillips III
 */

#include "timermodule.h"
#include "memmap.h"
#include "interrupt.h"

TimerModule::TimerModule(MemoryController & memory)
    : m_divider(memory.read(CPU_TIMER_DIV_ADDRESS)),
      m_counter(memory.read(CPU_TIMER_COUNTER_ADDRESS)),
      m_modulo(memory.read(CPU_TIMER_MODULO_ADDRESS)),
      m_control(memory.read(CPU_TIMER_CONTROL_ADDRESS)),
      m_memory(memory)
{
    reset();
}

void TimerModule::reset()
{
    m_divider = m_counter = m_modulo = m_control = 0x00;
}

void TimerModule::cycle()
{

}
