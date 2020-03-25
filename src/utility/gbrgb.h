#ifndef _GB_RGB_H
#define _GB_RGB_H

#include <cstdint>
#include <vector>
#include <sstream>
#include <iomanip>

namespace GB {
    struct RGB {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
        uint8_t alpha;

        RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
            : red(r), green(g), blue(b), alpha(a) { }

        RGB() : RGB(0, 0, 0, 0) { }

        RGB(const RGB & other)
            : RGB(other.red, other.blue, other.green, other.alpha) { }

        RGB & operator=(const RGB & other)
        {
            this->red   = other.red;
            this->green = other.green;
            this->blue  = other.blue;
            this->alpha = other.alpha;
            return *this;
        }

        bool operator==(const RGB & other)
        {
            return ((this->red == other.red) && (this->green == other.green)
                    && (this->blue == other.blue) && (this->alpha == other.alpha));
        }

        std::string toString() const
        {
            std::stringstream stream;
            stream << std::hex << std::setw(2) <<
                "r:0x" << int(this->red) << " " <<
                "g:0x" << int(this->green) << " " <<
                "b:0x" << int(this->blue) << " " <<
                "a:0x" << int(this->alpha);

            return stream.str();
        }
    };
};

typedef std::vector<GB::RGB> ColorArray;

#endif
