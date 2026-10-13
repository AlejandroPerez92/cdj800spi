#include "Cdj.h"

Cdj *volatile Cdj::instance_ = nullptr;

#if defined(ARDUINO_ARCH_AVR) && (defined(__AVR_ATmega328P__) || \
    defined(__AVR_ATmega168__) || defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__))
#include <Arduino.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

namespace {
#if defined(__AVR_ATmega2560__) || defined(__AVR_ATmega1280__)
const uint8_t selectBit = 0; // Mega SS = PB0 / digital53
#else
const uint8_t selectBit = 2; // Uno/Nano SS = PB2 / digital10
#endif
bool selected() { return (PINB & _BV(selectBit)) == 0; }
}

bool Cdj::begin() {
    if (instance_ != nullptr && instance_ != this) return false;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        SPCR = 0;
        instance_ = this;
        handler_.reset(); dirty_ = false;
        status_ = StatusMessage(); hasStatus_ = false;
        pressed_ = released_ = received_ = invalid_ = 0;
        jogDelta_ = 0; previousJog_ = 0; hardwareSelected_ = false;
        pinMode(SCK, INPUT); pinMode(MOSI, INPUT);
        pinMode(MISO, OUTPUT); pinMode(SS, INPUT_PULLUP);
        const uint8_t flags = SPSR;
        const uint8_t data = SPDR;
        (void)flags; (void)data;
        SPDR = 0;
        PCIFR = _BV(PCIF0);
        PCMSK0 |= _BV(selectBit);
        PCICR |= _BV(PCIE0);
        // SPI slave, MSB first, CPOL=1/CPHA=1 (mode3).
        SPCR = _BV(SPE) | _BV(SPIE) | _BV(CPOL) | _BV(CPHA);
        if (selected()) handleSelect();
    }
    return true;
}
void Cdj::end() {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        if (instance_ == this) {
            SPCR = 0;
            PCMSK0 &= ~_BV(selectBit);
            if (PCMSK0 == 0) PCICR &= ~_BV(PCIE0);
            pinMode(MISO, INPUT);
            hardwareSelected_ = false;
            instance_ = nullptr;
        }
    }
}
void Cdj::handleSPI() {
    const uint8_t received = SPDR;
    if (!hardwareSelected_) return;
    // Match the proven prototype's priority: reply first, then handle RX.
    // The response was prefetched while processing the previous byte.
    SPDR = handler_.nextByteToSend();
    __asm__ __volatile__("" ::: "memory");
    handler_.receiveByte(received);
    if (!selected()) {
        onDeselect(); hardwareSelected_ = false;
        SPDR = handler_.nextHeader();
    }
}
void Cdj::handleSelect() {
    const bool active = selected();
    if (active == hardwareSelected_) return;
    if (active) {
        hardwareSelected_ = true;
        SPDR = onSelect();
    } else {
        // PCINT0 precedes SPI_STC. Consume a pending final byte exactly once.
        if ((SPSR & _BV(SPIF)) != 0) {
            const uint8_t received = SPDR;
            handler_.receiveByte(received);
        }
        onDeselect(); hardwareSelected_ = false;
        SPDR = handler_.nextHeader();
    }
}
void Cdj::spiInterrupt() {
    Cdj *const instance = instance_;
    if (instance != nullptr) instance->handleSPI();
}
void Cdj::selectInterrupt() {
    Cdj *const instance = instance_;
    if (instance != nullptr) instance->handleSelect();
}

#ifndef CDJ800SPI_EXTERNAL_INTERRUPTS
ISR(SPI_STC_vect) { Cdj::spiInterrupt(); }
ISR(PCINT0_vect) { Cdj::selectInterrupt(); }
#endif

#else
bool Cdj::begin() { return false; }
void Cdj::end() {}
void Cdj::handleSPI() {}
void Cdj::handleSelect() {}
void Cdj::spiInterrupt() {}
void Cdj::selectInterrupt() {}
#endif
