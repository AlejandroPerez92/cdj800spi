#pragma once
#include <stdint.h>

// Change to 115200 ONLY when using a computer serial-to-MIDI bridge.
// DIN MIDI and USB MIDI via HIDUINO on the 16U2/8U2 use 31250.
// Serial carries binary MIDI, never debug text.
#ifndef CDJ800_MIDI_BAUD
#define CDJ800_MIDI_BAUD 31250
#endif

namespace MidiConfig {
constexpr uint8_t CHANNEL = 1;            // 1..16
constexpr uint8_t FIRST_BUTTON_NOTE = 36; // Button::Cue=36, ..., CallFwd=62
constexpr uint8_t PITCH_CC = 20;
constexpr uint8_t TOUCH_CC = 21;
constexpr uint8_t JOG_CC = 22;
constexpr uint8_t JOG_PRESSURE_CC = 23;

// Full raw domain by default. Measure the actual slider endpoints with
// PanelDemo, then replace these values to obtain the full MIDI 0..127 range.
// pitchReference is not used: its calibration meaning is still provisional.
constexpr uint16_t PITCH_RAW_MIN = 0;
constexpr uint16_t PITCH_RAW_MAX = 65535;
constexpr bool INVERT_PITCH = false;
constexpr bool INVERT_JOG = false;
constexpr uint16_t JOG_PULSES_PER_STEP = 1; // Increase to reduce sensitivity.

// These two physical interpretations still need checking on the panel.
constexpr bool SEND_TOUCH = false;        // raw byte 3 scaled to 0..127
constexpr bool SEND_JOG_PRESSURE = false; // jogFlags bit 0x10, 0/127
constexpr uint16_t ANALOG_INTERVAL_MS = 20;
constexpr uint16_t PANEL_TIMEOUT_MS = 2000;
}
