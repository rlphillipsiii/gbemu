#ifndef _JOYPAD_H
#define _JOYPAD_H

#include <cstdint>
#include <array>
#include <unordered_map>
#include <mutex>

#include "gameboyinterface.h"

class MemoryController;

class JoyPad {
public:
    explicit JoyPad(MemoryController & memory);
    ~JoyPad() = default;

    void cycle();
    
    void set(GameBoyInterface::JoyPadButton button);
    void clr(GameBoyInterface::JoyPadButton button);
    
private:
    enum ShadowSelect {
        SHADOW_DIRS    = 0x10,
        SHADOW_BUTTONS = 0x20,
    };

    static const std::unordered_map<ShadowSelect, int> SHADOW_MAP;
    static const std::unordered_map<GameBoyInterface::JoyPadButton, ShadowSelect> BUTTON_MAP;

    std::mutex m_lock;
    
    uint8_t & m_register;

    std::array<uint8_t, 2> m_shadow;
};

#endif
