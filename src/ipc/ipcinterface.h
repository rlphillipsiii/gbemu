#ifndef _IPC_INTERFACE_H
#define _IPC_INTERFACE_H

#include <cstdint>

class IpcInterface {
public:
    virtual bool start() = 0;
    virtual void stop() = 0;

    virtual bool rd(uint8_t & data) = 0;
    virtual bool wr(uint8_t data) = 0;
};

#endif
