/*
 * cpuclock.cpp
 *
 *  Created on: May 26, 2019
 *      Author: Robert Phillips III
 */

#include "cpuclock.h"

CpuClock::CpuClock()
    : m_cpu(m_memory),
      m_interrupt(false)
{
    m_thread.setObjectName("CPU_thread");

    moveToThread(&m_thread);
    m_thread.start();

    QMetaObject::invokeMethod(this, "run", Qt::QueuedConnection);
}

CpuClock::~CpuClock()
{
    stop();

    m_thread.quit();
    m_thread.wait();
}

void CpuClock::run()
{
    while (!m_interrupt.load()) {
        m_cpu.cycle();
    }
}

void CpuClock::stop()
{
    m_interrupt = true;
}
