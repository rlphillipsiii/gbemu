/*
 * cpuclock.h
 *
 *  Created on: May 26, 2019
 *      Author: Robert Phillips III
 */

#ifndef CPUCLOCK_H_
#define CPUCLOCK_H_

#include <QObject>
#include <QThread>
#include <QTimer>

#include <atomic>

#include "processor.h"
#include "memorycontroller.h"

class CpuClock : public QObject {
    Q_OBJECT

public:
    CpuClock();
    ~CpuClock();

    void stop();

public slots:
    void run();

private:
    QThread m_thread;

    MemoryController m_memory;
    Processor m_cpu;

    std::atomic<bool> m_interrupt;
};



#endif /* SRC_UI_CPUCLOCK_H_ */
