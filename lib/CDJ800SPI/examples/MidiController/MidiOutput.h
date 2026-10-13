#pragma once
#include <StatusMessage.h>
#include "MidiConfig.h"

// Example-only MIDI encoder. Fixed memory; no dependency on a MIDI library.
class MidiOutput {
public:
    static constexpr uint8_t BUTTON_COUNT = static_cast<uint8_t>(Button::Count);
    struct Packet { uint8_t status, data1, data2; };

    // Call immediately after cdj.update(), before its edge flags are cleared.
    void buttons(uint32_t presses, uint32_t releases, uint32_t held) {
        held_ = held;
        if (resetIndex_ < BUTTON_COUNT) return;
        for (uint8_t i = 0; i < BUTTON_COUNT; ++i) {
            const uint32_t bit = uint32_t(1) << i;
            const bool press = (presses & bit) != 0, release = (releases & bit) != 0;
            if (press && release) {
                // Two RX frames may contain both edges. The final state tells
                // us their order: tap=on/off; retrigger=off/on.
                const bool down = (held & bit) != 0;
                if (!enqueue(i, !down) || !enqueue(i, down)) return;
            } else if (press || release) {
                if (!enqueue(i, press)) return;
            }
        }
    }

    static uint8_t pitchValue(uint16_t raw) {
        if (raw <= MidiConfig::PITCH_RAW_MIN) return MidiConfig::INVERT_PITCH ? 127 : 0;
        if (raw >= MidiConfig::PITCH_RAW_MAX) return MidiConfig::INVERT_PITCH ? 0 : 127;
        const uint32_t span = uint32_t(MidiConfig::PITCH_RAW_MAX) - MidiConfig::PITCH_RAW_MIN;
        const uint8_t value = (uint32_t(raw - MidiConfig::PITCH_RAW_MIN) * 127 + span / 2) / span;
        return MidiConfig::INVERT_PITCH ? 127 - value : value;
    }

    void analog(uint16_t pitch, uint8_t touch, bool pressure) {
        pitch_ = pitchValue(pitch);
        touch_ = touch >> 1;
        pressure_ = pressure ? 127 : 0;
        haveAnalog_ = true;
    }

    void jog(int32_t delta) {
        if (delta > 0 && jogPending_ > INT32_MAX - delta) jogPending_ = INT32_MAX;
        else if (delta < 0 && jogPending_ < INT32_MIN - delta) jogPending_ = INT32_MIN;
        else jogPending_ += delta;
    }

    // On disconnect/overflow, explicitly release all mapped notes. While
    // recovering, retain the latest held snapshot and replay it afterwards.
    void releaseAll() {
        held_ = 0;
        startReset();
        haveAnalog_ = false;
        lastPitch_ = lastTouch_ = lastPressure_ = 255;
        jogPending_ = 0;
    }

    // Consume ONLY when the UART has room for an entire three-byte message.
    // Buttons take priority; continuous inputs coalesce to their latest value.
    bool next(Packet &packet) {
        if (resetIndex_ < BUTTON_COUNT) {
            packet = note(resetIndex_++, false);
            if (resetIndex_ == BUTTON_COUNT) {
                for (uint8_t i = 0; i < BUTTON_COUNT; ++i)
                    if (held_ & (uint32_t(1) << i)) enqueue(i, true);
            }
            return true;
        }
        if (read_ != write_) {
            const uint8_t event = events_[read_];
            read_ = (read_ + 1) & QUEUE_MASK;
            packet = note(event & 0x7f, (event & 0x80) != 0);
            return true;
        }
        if (haveAnalog_) {
            if (MidiConfig::SEND_JOG_PRESSURE && pressure_ != lastPressure_)
                return cc(packet, MidiConfig::JOG_PRESSURE_CC, pressure_, lastPressure_);
            if (pitch_ != lastPitch_)
                return cc(packet, MidiConfig::PITCH_CC, pitch_, lastPitch_);
            if (MidiConfig::SEND_TOUCH && touch_ != lastTouch_)
                return cc(packet, MidiConfig::TOUCH_CC, touch_, lastTouch_);
        }
        int32_t steps = jogPending_ / MidiConfig::JOG_PULSES_PER_STEP;
        if (steps == 0) return false;
        if (steps > 63) steps = 63;
        if (steps < -63) steps = -63;
        jogPending_ -= steps * MidiConfig::JOG_PULSES_PER_STEP;
        if (MidiConfig::INVERT_JOG) steps = -steps;
        // Relative two's complement: +1=1, -1=127, +63=63, -63=65.
        packet = {uint8_t(0xb0 | (MidiConfig::CHANNEL - 1)), MidiConfig::JOG_CC,
                  uint8_t(steps < 0 ? 128 + steps : steps)};
        return true;
    }

private:
    static constexpr uint8_t QUEUE_MASK = 63; // 63 usable button events
    uint8_t events_[64] = {};
    uint8_t read_ = 0, write_ = 0, resetIndex_ = BUTTON_COUNT;
    uint32_t held_ = 0;
    int32_t jogPending_ = 0;
    uint8_t pitch_ = 0, touch_ = 0, pressure_ = 0;
    uint8_t lastPitch_ = 255, lastTouch_ = 255, lastPressure_ = 255;
    bool haveAnalog_ = false;

    void startReset() { read_ = write_ = 0; resetIndex_ = 0; }
    bool enqueue(uint8_t button, bool down) {
        const uint8_t next = (write_ + 1) & QUEUE_MASK;
        if (next == read_) {
            startReset();
            return false;
        }
        events_[write_] = button | (down ? 0x80 : 0);
        write_ = next;
        return true;
    }
    static Packet note(uint8_t button, bool down) {
        return {uint8_t((down ? 0x90 : 0x80) | (MidiConfig::CHANNEL - 1)),
                uint8_t(MidiConfig::FIRST_BUTTON_NOTE + button), uint8_t(down ? 127 : 0)};
    }
    static bool cc(Packet &packet, uint8_t number, uint8_t value, uint8_t &last) {
        packet = {uint8_t(0xb0 | (MidiConfig::CHANNEL - 1)), number, value};
        last = value;
        return true;
    }
};

static_assert(MidiConfig::CHANNEL >= 1 && MidiConfig::CHANNEL <= 16, "MIDI channel must be 1..16");
static_assert(MidiConfig::FIRST_BUTTON_NOTE + MidiOutput::BUTTON_COUNT <= 128, "MIDI notes must fit 0..127");
static_assert(MidiConfig::PITCH_RAW_MIN < MidiConfig::PITCH_RAW_MAX, "Pitch endpoints must be increasing");
static_assert(MidiConfig::JOG_PULSES_PER_STEP > 0, "Jog divider must be positive");
static_assert(MidiConfig::PITCH_CC < 120 && MidiConfig::TOUCH_CC < 120 &&
              MidiConfig::JOG_CC < 120 && MidiConfig::JOG_PRESSURE_CC < 120,
              "Use CC 0..119; 120..127 are channel mode messages");
