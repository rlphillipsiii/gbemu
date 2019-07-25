#include <map>
#include <mutex>
#include <unordered_map>

#include "joypad.h"
#include "memorycontroller.h"
#include "memmap.h"

using std::unordered_map;
using std::mutex;
using std::lock_guard;

const unordered_map<JoyPad::ShadowSelect, int> JoyPad::SHADOW_MAP = {
    { SHADOW_DIRS,    0 },
    { SHADOW_BUTTONS, 1 },
};

const unordered_map<GameBoyInterface::JoyPadButton, JoyPad::ShadowSelect> JoyPad::BUTTON_MAP = {
    { GameBoyInterface::JOYPAD_A,      SHADOW_BUTTONS },
	{ GameBoyInterface::JOYPAD_B,      SHADOW_BUTTONS },
	{ GameBoyInterface::JOYPAD_SELECT, SHADOW_BUTTONS },
	{ GameBoyInterface::JOYPAD_START,  SHADOW_BUTTONS },
	{ GameBoyInterface::JOYPAD_RIGHT,  SHADOW_DIRS    },
	{ GameBoyInterface::JOYPAD_LEFT,   SHADOW_DIRS    },
	{ GameBoyInterface::JOYPAD_UP,     SHADOW_DIRS    },
    { GameBoyInterface::JOYPAD_DOWN,   SHADOW_DIRS    },
};

JoyPad::JoyPad(MemoryController & memory)
    : m_register(memory.read(JOYPAD_INPUT_ADDRESS))
{
    m_shadow[0] = m_shadow[1] = 0x0F;
}

void JoyPad::cycle()
{
    lock_guard<mutex> guard(m_lock);

    uint8_t state = 0x0F;
    
    if (!(m_register & SHADOW_BUTTONS)) {
        state = m_shadow[SHADOW_MAP.at(SHADOW_BUTTONS)];
    } else if (!(m_register & SHADOW_DIRS)) {
        state = m_shadow[SHADOW_MAP.at(SHADOW_DIRS)];
    }

    m_register = (m_register & 0xF0) | state;
}

void JoyPad::clr(GameBoyInterface::JoyPadButton button)
{
    lock_guard<mutex> guard(m_lock);

    ShadowSelect select = BUTTON_MAP.at(button);
    m_shadow[SHADOW_MAP.at(select)] |= button;
}

void JoyPad::set(GameBoyInterface::JoyPadButton button)
{
    lock_guard<mutex> guard(m_lock);

    ShadowSelect select = BUTTON_MAP.at(button);
    m_shadow[SHADOW_MAP.at(select)] &= ~button;
}
