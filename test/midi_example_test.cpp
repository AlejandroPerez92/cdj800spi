// Exercise the distributed sketch and real portable SPI library, with only
// the UART/clock replaced by a bounded host fixture.
#include <array>
#include <cassert>
#include <vector>
#include "MidiController.ino"

HardwareSerial Serial;
unsigned long fakeNow = 100;
unsigned long millis() { return fakeNow; }

using Bytes = std::array<uint8_t, 3>;
Bytes bytes(const MidiOutput::Packet &p) { return {{p.status, p.data1, p.data2}}; }
Bytes message(uint8_t family, uint8_t number, uint8_t value) {
    return {{uint8_t(family | (MidiConfig::CHANNEL - 1)), number, value}};
}
void expect(MidiOutput &output, uint8_t family, uint8_t number, uint8_t value) {
    MidiOutput::Packet packet;
    assert(output.next(packet));
    assert(bytes(packet) == message(family, number, value));
    assert(packet.data1 < 128 && packet.data2 < 128);
}
void empty(MidiOutput &output) { MidiOutput::Packet packet; assert(!output.next(packet)); }
uint8_t relative(int step) {
    if (MidiConfig::INVERT_JOG) step = -step;
    return uint8_t(step < 0 ? 128 + step : step);
}

void encoderTests() {
    const uint8_t first = MidiConfig::FIRST_BUTTON_NOTE;
    MidiOutput output;
    const uint32_t all = (uint32_t(1) << MidiOutput::BUTTON_COUNT) - 1;
    // Every button can tap within one update; all 54 ordered edges must survive.
    output.buttons(all, all, 0);
    for (uint8_t i = 0; i < MidiOutput::BUTTON_COUNT; ++i) {
        expect(output, 0x90, first + i, 127);
        expect(output, 0x80, first + i, 0);
    }
    empty(output);
    // Final down means release then press, including across the queue wrap.
    output.buttons(all, all, all);
    for (uint8_t i = 0; i < MidiOutput::BUTTON_COUNT; ++i) {
        expect(output, 0x80, first + i, 0);
        expect(output, 0x90, first + i, 127);
    }
    empty(output);
    // A congested UART must recover without leaving held/stuck MIDI notes.
    for (uint8_t i = 0; i < 64; ++i)
        output.buttons(i % 2 == 0 ? 1 : 0, i % 2 != 0 ? 1 : 0, i % 2 == 0 ? 1 : 0);
    output.buttons(1, 0, 1); // Latest held state while resetting.
    for (uint8_t i = 0; i < MidiOutput::BUTTON_COUNT; ++i)
        expect(output, 0x80, first + i, 0);
    expect(output, 0x90, first, 127);
    empty(output);

    const uint8_t low = MidiConfig::INVERT_PITCH ? 127 : 0, high = 127 - low;
    assert(MidiOutput::pitchValue(0) == low);
    assert(MidiOutput::pitchValue(MidiConfig::PITCH_RAW_MIN) == low);
    assert(MidiOutput::pitchValue(MidiConfig::PITCH_RAW_MAX) == high);
    assert(MidiOutput::pitchValue(65535) == high);
    for (uint32_t raw = 1; raw <= 65535; ++raw) {
        const uint8_t before = MidiOutput::pitchValue(raw - 1), after = MidiOutput::pitchValue(raw);
        assert(after < 128);
        assert(MidiConfig::INVERT_PITCH ? before >= after : before <= after);
    }
    output.analog(MidiConfig::PITCH_RAW_MIN, 255, true);
    output.analog(MidiConfig::PITCH_RAW_MAX, 0, false); // Coalesce analog updates.
    output.buttons(1, 0, 1);
    expect(output, 0x90, first, 127); // Notes have priority over analog data.
    if (MidiConfig::SEND_JOG_PRESSURE) expect(output, 0xb0, MidiConfig::JOG_PRESSURE_CC, 0);
    expect(output, 0xb0, MidiConfig::PITCH_CC, high);
    if (MidiConfig::SEND_TOUCH) expect(output, 0xb0, MidiConfig::TOUCH_CC, 0);
    output.analog(MidiConfig::PITCH_RAW_MAX, 0, false);
    empty(output); // Unchanged analog inputs do not flood the UART.
    output.analog(MidiConfig::PITCH_RAW_MIN, 255, true);
    if (MidiConfig::SEND_JOG_PRESSURE) expect(output, 0xb0, MidiConfig::JOG_PRESSURE_CC, 127);
    expect(output, 0xb0, MidiConfig::PITCH_CC, low);
    if (MidiConfig::SEND_TOUCH) expect(output, 0xb0, MidiConfig::TOUCH_CC, 127);
    empty(output);

    MidiOutput wheel;
    const int32_t divider = MidiConfig::JOG_PULSES_PER_STEP;
    wheel.jog(divider - 1); empty(wheel);
    wheel.jog(1); expect(wheel, 0xb0, MidiConfig::JOG_CC, relative(1));
    for (int sign : {1, -1}) {
        wheel.jog(sign * 130 * divider);
        for (int step : {63, 63, 4}) expect(wheel, 0xb0, MidiConfig::JOG_CC, relative(sign * step));
        empty(wheel);
    }
    wheel.jog(INT32_MAX); wheel.jog(INT32_MAX);
    expect(wheel, 0xb0, MidiConfig::JOG_CC, relative(63));
    wheel.jog(INT32_MIN);
    expect(wheel, 0xb0, MidiConfig::JOG_CC, relative(-63));
    if (divider == 1) expect(wheel, 0xb0, MidiConfig::JOG_CC, relative(-1));
    empty(wheel);
    wheel.jog(1000); wheel.releaseAll();
    for (uint8_t i = 0; i < MidiOutput::BUTTON_COUNT; ++i) expect(wheel, 0x80, first + i, 0);
    empty(wheel);
}

void receive(bool cue, uint16_t jog) {
    uint8_t frame[20] = {1, 16};
    frame[4] = MidiConfig::PITCH_RAW_MIN >> 8;
    frame[5] = MidiConfig::PITCH_RAW_MIN & 255;
    frame[8] = jog >> 8; frame[9] = jog & 255;
    frame[14] = cue ? 0x80 : 0;
    frame[19] = StatusMessage::checksum(frame);
    cdj.onSelect();
    for (uint8_t value : frame) cdj.onTransfer(value);
    cdj.onDeselect();
}
std::vector<Bytes> drain(unsigned loops = 100) {
    Serial.room = 3;
    for (unsigned i = 0; i < loops; ++i) loop();
    assert(Serial.sent.size() % 3 == 0);
    std::vector<Bytes> packets;
    for (size_t i = 0; i < Serial.sent.size(); i += 3)
        packets.push_back({{Serial.sent[i], Serial.sent[i + 1], Serial.sent[i + 2]}});
    Serial.sent.clear();
    return packets;
}
void sketchTests() {
    setup();
    assert(Serial.baud == CDJ800_MIDI_BAUD);
    Serial.room = 0;
    receive(true, 100); receive(false, 100); loop();
    assert(cdj.receivedMessages() == 2 && Serial.sent.empty());
    assert(panelOnline); // Both SPI frames processed even with UART full.
    auto packets = drain();
    std::vector<Bytes> expected = {
        message(0x90, MidiConfig::FIRST_BUTTON_NOTE, 127),
        message(0x80, MidiConfig::FIRST_BUTTON_NOTE, 0)
    };
    if (MidiConfig::SEND_JOG_PRESSURE) expected.push_back(message(0xb0, MidiConfig::JOG_PRESSURE_CC, 0));
    expected.push_back(message(0xb0, MidiConfig::PITCH_CC, MidiConfig::INVERT_PITCH ? 127 : 0));
    if (MidiConfig::SEND_TOUCH) expected.push_back(message(0xb0, MidiConfig::TOUCH_CC, 0));
    assert(packets == expected);

    fakeNow += MidiConfig::PANEL_TIMEOUT_MS + 1;
    packets = drain();
    assert(!panelOnline && packets.size() == MidiOutput::BUTTON_COUNT);
    for (uint8_t i = 0; i < MidiOutput::BUTTON_COUNT; ++i)
        assert(packets[i] == message(0x80, MidiConfig::FIRST_BUTTON_NOTE + i, 0));
    receive(true, 60000); // Reconnect with a different counter and held CUE.
    packets = drain();
    assert(panelOnline && packets.front() == message(0x90, MidiConfig::FIRST_BUTTON_NOTE, 127));
    for (auto p : packets) assert(p != message(0xb0, MidiConfig::JOG_CC, p[2]));
    receive(false, 60000 + MidiConfig::JOG_PULSES_PER_STEP);
    packets = drain();
    assert(packets.size() == 2);
    assert(packets[0] == message(0x80, MidiConfig::FIRST_BUTTON_NOTE, 0));
    assert(packets[1] == message(0xb0, MidiConfig::JOG_CC, relative(1)));
}

int main() { encoderTests(); sketchTests(); }
