#include <Arduino.h>
#include <CDJ800SPI.h>
#include "MidiOutput.h"

// Arduino Uno R3: panel uses D10..D13, MIDI OUT uses hardware TX on D1.
// See README.md for the DIN circuit and MIDI Learn/relative jog settings.
CDJ800SPI cdj;
MidiOutput midi;
bool panelOnline = false, playing = false;
uint8_t jogPosition = 1;
unsigned long lastPanelFrame = 0, lastAnalogUpdate = 0;

void collectMidiInputs() {
    const StatusMessage &input = cdj.status();
    uint32_t presses = 0, releases = 0;
    for (uint8_t i = 0; i < MidiOutput::BUTTON_COUNT; ++i) {
        const Button button = static_cast<Button>(i);
        if (cdj.pressed(button)) presses |= uint32_t(1) << i;
        if (cdj.released(button)) releases |= uint32_t(1) << i;
    }
    // Also restore held notes on reconnection, even if the library's last
    // snapshot already had those buttons down.
    if (!panelOnline) presses |= input.buttons();
    midi.buttons(presses, releases, input.buttons());
    // A counter jump during disconnection must not become a jog movement.
    if (panelOnline) midi.jog(cdj.jogDelta());
}

void updatePanelFeedback() {
    const StatusMessage &input = cdj.status();
    if (cdj.pressed(Button::Play)) playing = !playing;
    cdj.setIndicator(Indicator::Play, playing);
    cdj.setIndicator(Indicator::Cue, input.btnCue);
    cdj.setIndicator(Indicator::Reverse, input.btnRev);
    cdj.setIndicator(Indicator::CueIn, input.btnCueIn);
    cdj.setIndicator(Indicator::CueOut, input.btnCueOut);
    cdj.setIndicator(Indicator::Beat1, input.btnBeat1);
    cdj.setIndicator(Indicator::Beat2, input.btnBeat2);
    cdj.setIndicator(Indicator::Beat4, input.btnBeat4);
    cdj.setIndicator(Indicator::Beat8, input.btnBeat8);
    cdj.setIndicator(Indicator::Vinyl, input.btnVinyl);
    cdj.setIndicator(Indicator::MasterTempo, input.btnMasterTempo);
    cdj.setIndicator(Indicator::QuickReturn, input.btnQuickReturn);
    int32_t position = int32_t(jogPosition) - 1 + (panelOnline ? cdj.jogDelta() : 0);
    position %= 135;
    if (position < 0) position += 135;
    jogPosition = uint8_t(position + 1);
    cdj.setDisplayJogPos(jogPosition);
}

void serviceMidi() {
    // Serial.write() would wait if the TX ring were full. Reserve the whole
    // packet first so loop() can keep draining the panel's SPI receive queue.
    if (Serial.availableForWrite() < 3) return;
    MidiOutput::Packet packet;
    if (midi.next(packet)) {
        const uint8_t bytes[] = {packet.status, packet.data1, packet.data2};
        Serial.write(bytes, sizeof(bytes));
    }
}

void setup() {
    Serial.begin(CDJ800_MIDI_BAUD);
    cdj.setText("MIDI CTRL");
    cdj.setTrackNumber(MidiConfig::CHANNEL);
    cdj.setDisplayJogPos(jogPosition);
    if (!cdj.begin()) cdj.setText("SPI ERROR");
    // Do not Serial.print(): text on this UART would corrupt the MIDI stream.
}

void loop() {
    const unsigned long now = millis();
    if (cdj.update()) {
        collectMidiInputs();
        if (!panelOnline) {
            cdj.setText("MIDI CTRL");
            const StatusMessage &input = cdj.status();
            midi.analog(input.pitch, input.touch, input.jogPressure);
            lastAnalogUpdate = now;
        }
        updatePanelFeedback();
        panelOnline = true;
        lastPanelFrame = now;
    }
    if (panelOnline && now - lastAnalogUpdate >= MidiConfig::ANALOG_INTERVAL_MS) {
        const StatusMessage &input = cdj.status();
        midi.analog(input.pitch, input.touch, input.jogPressure);
        cdj.setProgressPercentage(leftToRight, uint16_t(MidiOutput::pitchValue(input.pitch)) * 100 / 127);
        lastAnalogUpdate = now;
    }
    if (panelOnline && now - lastPanelFrame >= MidiConfig::PANEL_TIMEOUT_MS) {
        midi.releaseAll();
        panelOnline = playing = false;
        cdj.clearDisplay();
        cdj.setText("NO PANEL");
        cdj.setTrackNumber(MidiConfig::CHANNEL);
    }
    cdj.commit();
    serviceMidi();
    // No delay(), SoftwareSerial, MIDIUSB, or additional interrupt handlers.
}
