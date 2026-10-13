// Portable Arduino regression checks with fixed E02 panel vectors.
#include <CDJ800SPI.h>
#include <algorithm>
#include <array>
#include <cassert>
#include <iostream>
#include <cstring>

#include "arduino_panel_vectors.h"

uint8_t fixtureChecksum(const uint8_t *data) {
    unsigned sum = 0;
    for (uint8_t i = 0; i < 19; ++i) sum += data[i];
    return sum == 0 ? 0 : uint8_t(1 + (sum - 1) % 255);
}
using Frame = std::array<uint8_t, 20>;
Frame frame(const Cdj &cdj, uint8_t group) {
    Frame f{}; assert(cdj.getFrame(group, f.data())); return f;
}
Frame input(uint8_t cue = 0, uint16_t jog = 0) {
    Frame f{}; f[0] = 1; f[1] = 16; f[14] = cue;
    f[8] = jog >> 8; f[9] = jog;
    f[19] = fixtureChecksum(f.data()); return f;
}
Frame exchange(Cdj &cdj, const Frame &rx) {
    Frame tx{};
    uint8_t next = cdj.onSelect();
    for (size_t i = 0; i < 20; ++i) {
        tx[i] = next; next = cdj.onTransfer(rx[i]);
    }
    assert(next == 0); cdj.onDeselect(); return tx;
}
void checksumsAndInputs() {
    Frame raw{};
    assert(StatusMessage::checksum(raw.data()) == 0);
    raw[0] = 255;
    assert(StatusMessage::checksum(raw.data()) == 255);
    raw[19] = 0; assert(!StatusMessage::isValidCrc(raw.data()));
    for (unsigned sum = 0; sum <= 19 * 255; ++sum) {
        raw.fill(0);
        unsigned remainder = sum;
        for (unsigned i = 0; i < 19; ++i) {
            raw[i] = std::min(remainder, 255u); remainder -= raw[i];
        }
        assert(StatusMessage::checksum(raw.data()) == fixtureChecksum(raw.data()));
    }
    raw = input();
    raw[3] = 0x7a; raw[4] = 0x12; raw[5] = 0x34;
    raw[6] = 0xab; raw[7] = 0xcd; raw[8] = 0xff; raw[9] = 0xfe;
    raw[10] = 0x56; raw[11] = 0x78; raw[12] = 0xf0; raw[13] = 0x99;
    StatusMessage status = StatusMessage::createMessage(raw.data());
    assert(status.touch == 0x7a && status.pitch == 0x1234 && status.pitchReference == 0xabcd);
    assert(status.jogPulse == 0xfffe && status.jogSpeed == 0x5678 && status.auxiliary13 == 0x99);
    assert(status.jogActive && status.jogForward && status.jogFlag20 && status.jogPressure);
    const uint8_t bytes[] = {14,14,14,14,14,14,14,14,15,15,16,16,16,16,17,17,17,17,17,17,17,18,18,18,18,18,18};
    const uint8_t masks[] = {128,32,64,16,8,4,2,1,2,1,8,4,2,1,64,32,16,8,4,2,1,32,16,8,4,2,1};
    for (uint8_t b = 0; b < static_cast<uint8_t>(Button::Count); ++b) {
        raw = input(); raw[bytes[b]] = masks[b];
        status = StatusMessage::createMessage(raw.data());
        assert(status.buttons() == (uint32_t(1) << b));
    }
    assert(!StatusMessage().btnCue && !StatusMessage().jogPressure);
}
void displays() {
    Cdj cdj;
    cdj.setText("HELLO CDJ");
    for (uint8_t g = 0; g < 5; ++g)
        assert(std::memcmp(frame(cdj, g).data() + 1, panel_fixture::text[g], 10) == 0);
    cdj.setText("AB"); Cdj padded; padded.setText("AB       ");
    for (uint8_t g = 0; g < 5; ++g) assert(frame(cdj, g) == frame(padded, g));
    cdj.setText(nullptr); padded.setText("         ");
    for (uint8_t g = 0; g < 5; ++g) assert(frame(cdj, g) == frame(padded, g));
    for (const auto &value : panel_fixture::bpm) {
        cdj.setBpm(value.input);
        assert(std::memcmp(frame(cdj, 3).data() + 11, value.digits, 3) == 0);
    }
    for (const auto &value : panel_fixture::tempo) {
        assert(cdj.setTempo(value.input, value.wide, value.master));
        assert(frame(cdj, 1)[11] == value.flags);
        assert(std::memcmp(frame(cdj, 2).data() + 11, value.digits, 3) == 0);
    }
    Frame before = frame(cdj, 2);
    assert(!cdj.setTempo(1601)); assert(frame(cdj, 2) == before);
    assert(!cdj.setTempo(-1601)); assert(frame(cdj, 2) == before);
    assert(!cdj.setTempo(10001, true)); assert(frame(cdj, 2) == before);
    for (const auto &value : panel_fixture::tracks) {
        assert(cdj.setTrackNumber(value.input));
        assert(std::memcmp(frame(cdj, 4).data() + 11, value.digits, 3) == 0);
    }
    before = frame(cdj, 4);
    assert(!cdj.setTrackNumber(1000)); assert(frame(cdj, 4) == before);
    cdj.clearDisplay(); cdj.setDisplaySeparator(on);
    for (const auto &value : panel_fixture::bars) {
        assert(cdj.setProgressSegments(value.position));
        const Frame f = frame(cdj, 5);
        for (uint8_t j = 0; j < 5; ++j)
            assert(f[5 + j] == (value.bytes[j] | (j == 0 ? 4 : 0)));
    }
    cdj.setProgressPercentage(leftToRight, 100); cdj.setProgressPercentage(leftToRight, 0);
    assert((frame(cdj, 5)[5] & 0xf8) == 0 && frame(cdj, 5)[6] == 0);
    for (const auto &value : panel_fixture::marks) {
        assert(cdj.setCueMarkers(&value.position, 1));
        const Frame f = frame(cdj, 5);
        for (uint8_t j = 0; j < 5; ++j)
            assert(f[1 + j] == (value.bytes[j] | (j == 4 ? 4 : 0)));
    }
    const uint8_t invalid = 34;
    assert(!cdj.setCueMarkers(&invalid, 1)); assert(!cdj.setProgressSegments(34));
    before = frame(cdj, 5);
    assert(!cdj.setRawByte(6, 1, 255)); assert(!cdj.setRawByte(0, 0, 255));
    assert(!cdj.setRawByte(0, 17, 255)); assert(frame(cdj, 5) == before);
    cdj.clearDisplay();
    for (uint8_t i = 0; i < static_cast<uint8_t>(Indicator::Count); ++i)
        cdj.setIndicator(static_cast<Indicator>(i), true);
    assert(frame(cdj, 5)[12] == 0xfc && frame(cdj, 5)[13] == 0xff);
    for (uint8_t i = 0; i < static_cast<uint8_t>(Indicator::Count); ++i)
        cdj.setIndicator(static_cast<Indicator>(i), false);
    assert(frame(cdj, 5)[12] == 0 && frame(cdj, 5)[13] == 0);
    cdj.setDisplayJogPos(255);
    for (uint8_t g = 0; g < 6; ++g) assert(frame(cdj, g)[17] == 135);
    cdj.setEjectAnimation(); assert(frame(cdj, 0)[17] == 138);
    cdj.clearBpm(); cdj.clearTempo(); cdj.clearTrackNumber(); cdj.clearDisplayJog();
}
void transmitBeforeReceive() {
    uint8_t drafts[6][20] = {};
    for (uint8_t g = 0; g < 6; ++g)
        for (uint8_t i = 1; i <= 17; ++i) drafts[g][i] = uint8_t(7 + 31 * g + i);
    SPIMessageHandler handler(drafts);
    const Frame rx = input(0x80, 0xabcd);
    for (uint8_t transaction = 0; transaction < 7; ++transaction) {
        const uint8_t group = transaction % 6;
        Frame expected{};
        expected[0] = group << 4;
        for (uint8_t i = 1; i <= 17; ++i) {
            expected[i] = drafts[group][i];
            expected[18] += expected[i];
        }
        assert(handler.select() == expected[0]);
        for (uint8_t i = 0; i < 20; ++i) {
            // This is the AVR order: get the response BEFORE handing over MOSI.
            const uint8_t next = i < 19 ? expected[i + 1] : 0;
            assert(handler.nextByteToSend() == next);
            assert(handler.nextByteToSend() == next); // reading does not advance
            handler.receiveByte(rx[i]);
        }
        assert(!handler.isMessageAvailable());
        handler.deselect();
        Message message;
        assert(handler.popMessage(message));
        assert(std::memcmp(message.content, rx.data(), 20) == 0);
        assert(handler.nextByteToSend() == 0);
        assert(handler.transfer(0xff) == 0); // deselected: no stale response
    }
    handler.select();
    for (unsigned i = 0; i < 300; ++i) {
        if (i >= 19) assert(handler.nextByteToSend() == 0);
        handler.receiveByte(0xaa);
    }
    handler.deselect();
    assert(handler.framingErrors() == 1 && !handler.isMessageAvailable());
    assert(handler.select() == 0); // broken transaction restarts the cache
    assert(handler.nextByteToSend() == drafts[0][1]);
    handler.deselect();
}
void transport() {
    Cdj cdj;
    uint16_t words[49];
    const uint8_t *expected = panel_fixture::transport;
    for (unsigned i = 0; i < 49; ++i) words[i] = 0x1234 + 7919 * i;
    for (uint8_t g = 0; g < 6; ++g) for (uint8_t i = 0; i < 16; ++i)
        assert(cdj.setRawByte(g, i + 1, (i & 1) ? words[8 * g + i / 2] : words[8 * g + i / 2] >> 8));
    words[48] = 0xa587; cdj.setDisplayJogPos(135);
    assert(cdj.commit());
    for (uint8_t g = 0; g < 7; ++g) {
        Frame tx = exchange(cdj, input());
        assert(std::memcmp(tx.data(), expected + 20 * (g % 6), 20) == 0);
        cdj.update();
    }
    // Mid-frame publication: the rest of this six-frame cycle stays unchanged.
    uint8_t next = cdj.onSelect(); assert(next == 0x10);
    for (uint8_t i = 0; i < 20; ++i) {
        assert(next == expected[20 + i]);
        next = cdj.onTransfer(input()[i]);
        if (i == 3) {
            cdj.clearDisplay(); assert(cdj.commit());
            cdj.setBpm(128); assert(!cdj.commit());
        }
    }
    cdj.onDeselect();
    for (uint8_t g = 2; g < 6; ++g) {
        Frame tx = exchange(cdj, input());
        assert(std::memcmp(tx.data(), expected + 20 * g, 20) == 0); cdj.update();
    }
    Frame tx = exchange(cdj, input()); // first new snapshot is entirely blank
    assert(tx == Frame{}); cdj.update(); // schedules the waiting BPM update
    for (uint8_t g = 1; g < 6; ++g) {
        tx = exchange(cdj, input());
        for (uint8_t i = 1; i < 20; ++i) assert(tx[i] == 0); cdj.update();
    }
    for (uint8_t g = 0; g < 4; ++g) { tx = exchange(cdj, input()); cdj.update(); }
    assert(tx[11] == 0xfe && tx[12] == 0xd6 && tx[13] == 0x24);
    // Every short CS selection restarts TX at group zero; no partial RX packet.
    for (uint8_t length = 0; length < 20; ++length) {
        cdj.onSelect(); for (uint8_t i = 0; i < length; ++i) cdj.onTransfer(1);
        cdj.onDeselect(); assert(!cdj.update());
        assert(exchange(cdj, input())[0] == 0); cdj.update();
    }
    assert(cdj.framingErrors() == 20);
    cdj.onSelect(); for (unsigned i = 0; i < 300; ++i) cdj.onTransfer(1); cdj.onDeselect();
    assert(cdj.framingErrors() == 21 && !cdj.update());
    Cdj queue;
    exchange(queue, input(0x80)); exchange(queue, input(0)); exchange(queue, input(0x80));
    assert(queue.droppedMessages() == 1); assert(queue.update());
    assert(queue.pressed(Button::Cue) && queue.released(Button::Cue) && !queue.status().btnCue);
    assert(!queue.update() && !queue.pressed(Button::Cue));
    Frame bad = input(0x80); bad[19] ^= 1; exchange(queue, bad); queue.update();
    assert(queue.invalidMessages() == 1 && !queue.status().btnCue);
    bad = input(); bad[0] = 2; bad[19] = fixtureChecksum(bad.data());
    exchange(queue, bad); queue.update(); assert(queue.invalidMessages() == 2);
    assert(Cdj::jogDifference(0, 65535) == 1 && Cdj::jogDifference(65535, 0) == -1);
    assert(Cdj::jogDifference(32767, 0) == -32769 && Cdj::jogDifference(0, 32767) == 32769);
    Cdj jog;
    exchange(jog, input(0, 65535)); jog.update(); assert(jog.jogDelta() == 0);
    exchange(jog, input(0, 0)); jog.update(); assert(jog.jogDelta() == 1);
    exchange(jog, input(0, 65535)); jog.update(); assert(jog.jogDelta() == -1);
}
int main() {
    checksumsAndInputs(); displays(); transmitBeforeReceive(); transport();
    std::cout << "PASS: Arduino display/checksum vectors, CS framing, coherent publication, input events and queue overflow\n";
}
