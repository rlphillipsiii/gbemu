#ifndef _PIPE_LINK_H
#define _PIPE_LINK_H

#include <string>

#include "consolelink.h"

class MemoryController;

class PipeLink : public ConsoleLink {
public:
    PipeLink(MemoryController & memory);
    ~PipeLink();

    void transfer(uint8_t value) override;

private:
    static const std::string PIPE_NAME;

    bool m_valid;
};

#endif
