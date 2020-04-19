#include <string>
#include <cstdint>
#include <sstream>
#include <cstdio>
#include <iomanip>

#include "logging.h"
#include "gbproc.h"

using std::string;
using std::stringstream;

namespace GB {
    void Command::print() const
    {
        TRACE("%s\n", str().c_str());
    }

    string Command::str() const
    {
        constexpr const int size = 1024;
        char buffer[size];

        stringstream temp;
        temp << ((flags & ZERO_FLAG_MASK)       ? "Z" : "-");
        temp << ((flags & NEG_FLAG_MASK)        ? "N" : "-");
        temp << ((flags & HALF_CARRY_FLAG_MASK) ? "H" : "-");
        temp << ((flags & CARRY_FLAG_MASK)      ? "C" : "-");

        stringstream stream;
        std::sprintf(buffer, "PC:0x%04x F:%s SP:0x%04x IME:%d IM:0x%02x INT:0x%02x | ",
                     pc, temp.str().c_str(), sp, int(ints), iMask, iStatus);

        std::sprintf(buffer, "%s %s (0x%02x): ", buffer, operation->name.c_str(), opcode);
        for (int8_t i = operation->length - 2; i >= 0; i--) {
            std::sprintf(buffer, "%s %02x", buffer, operands[i]);
        }
        std::sprintf(buffer, "%s\n", buffer);

        stream << buffer;

        std::sprintf(buffer,
                "    A:0x%02x BC:0x%04x DE:0x%04x HL:0x%04x ROM:%d RAM:%d SL:%d",
                a, bc, de, hl, romBank, ramBank, scanline);

        stream << buffer;
        return stream.str();
    }

    string Command::status() const
    {
        stringstream stream;
        stream << std::setfill('0') << std::setw(4) << std::hex;

        stream << "SP:0x" << int(sp);
        stream << std::setw(2);
        stream << " IME:" << int(ints) << " IM:0x" << int(iMask) << " INT:0x" << int(iStatus);
        stream << " ROM:" << std::to_string(int(romBank)) << " RAM:" << std::to_string(int(ramBank));
        return stream.str();
    }

    string Command::registers() const
    {
        stringstream stream;
        stream << std::setfill('0') << std::setw(2) << std::hex;

        stream << "A:0x" << int(a) << " F:";
        stream << ((flags & ZERO_FLAG_MASK)       ? "Z" : "-");
        stream << ((flags & NEG_FLAG_MASK)        ? "N" : "-");
        stream << ((flags & HALF_CARRY_FLAG_MASK) ? "H" : "-");
        stream << ((flags & CARRY_FLAG_MASK)      ? "C" : "-");

        stream << std::setw(4);
        stream << " BC:0x" << int(bc) << " DE:0x" << int(de) << " HL:0x" << int(hl);
        stream << std::setw(2);
        stream << " STAT:0x" << int(stat) << " SL:" << std::to_string(int(scanline));
        return stream.str();
    }

    string Command::abbrev() const
    {
        constexpr const int size = 1024;
        char buffer[size];

        std::sprintf(buffer, "%s (0x%02x): ", operation->name.c_str(), opcode);
        for (int8_t i = operation->length - 2; i >= 0; i--) {
            std::sprintf(buffer, "%s %02x", buffer, operands[i]);
        }

        return string(buffer);
    }

}
