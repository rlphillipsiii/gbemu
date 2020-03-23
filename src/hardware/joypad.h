#ifndef _JOYPAD_H
#define _JOYPAD_H

#include <cstdint>
#include <array>
#include <unordered_map>
#include <atomic>

#include "gameboyinterface.h"

class MemoryController;

class JoyPad final {
public:
    explicit JoyPad(MemoryController & memory);
    ~JoyPad() = default;

    void cycle(uint8_t ticks);

    void set(GameBoyInterface::JoyPadButton button);
    void clr(GameBoyInterface::JoyPadButton button);

private:
    static constexpr uint8_t BUTTONS_IDLE = 0x0F;

    enum ShadowSelect {
        SHADOW_DIRS    = 0x10,
        SHADOW_BUTTONS = 0x20,
    };

    static const std::unordered_map<ShadowSelect, int> SHADOW_MAP;
    static const std::unordered_map<GameBoyInterface::JoyPadButton, ShadowSelect> BUTTON_MAP;

    uint8_t & m_register;

    std::array<std::atomic<uint8_t>, 2> m_shadow;
};

#endif
