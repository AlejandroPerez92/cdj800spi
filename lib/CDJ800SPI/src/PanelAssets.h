#pragma once
#include <stdint.h>
#if defined(__AVR__)
#include <avr/pgmspace.h>
#define CDJ800_FLASH PROGMEM
#else
#define CDJ800_FLASH
#endif

namespace cdj800_detail {
extern const uint8_t glyphs[2560] CDJ800_FLASH;
extern const uint8_t barPatterns[204] CDJ800_FLASH;
extern const uint8_t markPatterns[170] CDJ800_FLASH;
inline uint8_t readAsset(const uint8_t *address) {
#if defined(__AVR__)
    return pgm_read_byte(address);
#else
    return *address;
#endif
}
}
