#include "clockinterface.h"

class ClockStub : public ClockInterface {
public:
    ClockStub() : m_ticks(0) { }
    ~ClockStub() = default;

    void tick(uint8_t ticks) override { m_ticks += ticks; }

    void setSpeed() { }
    void reset() { m_ticks = 0; }

private:
    uint32_t m_ticks;
};
