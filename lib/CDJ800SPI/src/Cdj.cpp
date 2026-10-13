#include "Cdj.h"
#include "PanelAssets.h"
#include <string.h>

using cdj800_detail::readAsset;
namespace {
const uint8_t digits[11] = {0xee,0x24,0xd6,0xb6,0x3c,0xba,0xfa,0x26,0xfe,0xbe,0};
uint32_t buttonMask(Button b) {
    return static_cast<uint8_t>(b) < static_cast<uint8_t>(Button::Count)
        ? uint32_t(1) << static_cast<uint8_t>(b) : 0;
}
}

Cdj::Cdj() : commands_{}, handler_(commands_), status_(), dirty_(false),
    hasStatus_(false), hardwareSelected_(false), pressed_(0), released_(0), received_(0), invalid_(0),
    jogDelta_(0), previousJog_(0) {}

void Cdj::write(uint8_t group, uint8_t index, uint8_t value) {
    if (commands_[group][index] != value) {
        commands_[group][index] = value; dirty_ = true;
    }
}
bool Cdj::commit() {
    if (!dirty_) return true;
    if (!handler_.publish()) return false;
    dirty_ = false;
    return true;
}
bool Cdj::update() {
    pressed_ = released_ = 0; jogDelta_ = 0;
    bool changed = false;
    Message message;
    // Bounded work even if the producer is continuously receiving.
    for (uint8_t i = 0; i < 2 && handler_.popMessage(message); ++i) {
        if (!StatusMessage::isValid(message.content)) { ++invalid_; continue; }
        const StatusMessage next = StatusMessage::createMessage(message.content);
        const uint32_t oldButtons = status_.buttons(), newButtons = next.buttons();
        pressed_ |= newButtons & ~oldButtons;
        released_ |= oldButtons & ~newButtons;
        if (hasStatus_) jogDelta_ += jogDifference(next.jogPulse, previousJog_);
        previousJog_ = next.jogPulse;
        status_ = next; hasStatus_ = changed = true; ++received_;
    }
    commit();
    return changed;
}
bool Cdj::pressed(Button b) const { return (pressed_ & buttonMask(b)) != 0; }
bool Cdj::released(Button b) const { return (released_ & buttonMask(b)) != 0; }
int32_t Cdj::jogDifference(uint16_t current, uint16_t previous) {
    int32_t delta = int32_t(current) - int32_t(previous);
    if (current > previous && delta >= 0x7fff) delta -= 0x10000L;
    else if (current < previous && -delta >= 0x7fff) delta += 0x10000L;
    return delta;
}
void Cdj::clearDisplay() {
    for (uint8_t g = 0; g < 6; ++g)
        for (uint8_t i = 1; i <= 17; ++i) write(g, i, 0);
}
void Cdj::setText(const char *text) {
    uint8_t glyphIndices[9];
    bool finished = text == nullptr;
    for (uint8_t i = 0; i < 9; ++i) {
        if (!finished && *text == '\0') finished = true;
        glyphIndices[i] = finished ? uint8_t(' ') : static_cast<uint8_t>(*text++);
    }
    setGlyphs(glyphIndices);
}
void Cdj::setGlyphs(const uint8_t indices[9]) {
    if (indices == nullptr) return;
    for (uint8_t group = 0; group < 5; ++group) {
        const uint8_t right = 8 - 2 * group;
        for (uint8_t j = 0; j < 10; ++j) {
            uint8_t value = readAsset(cdj800_detail::glyphs + uint16_t(indices[right]) * 10 + j);
            if (group < 4)
                value |= readAsset(cdj800_detail::glyphs + uint16_t(indices[right - 1]) * 10 + j) >> 1;
            write(group, j + 1, value);
        }
    }
}
bool Cdj::setTrackNumber(uint16_t track) {
    if (track > 999) return false;
    write(4, 11, digits[track % 10]);
    write(4, 12, track >= 10 ? digits[(track / 10) % 10] : 0);
    write(4, 13, track >= 100 ? digits[track / 100] : 0);
    return true;
}
void Cdj::clearTrackNumber() { for (uint8_t i = 11; i <= 13; ++i) write(4, i, 0); }
void Cdj::setBpm(uint16_t bpm) {
    if (bpm > 399) bpm = 399;
    write(3, 11, digits[bpm % 10]);
    write(3, 12, bpm >= 10 ? digits[(bpm / 10) % 10] : 0);
    write(3, 13, bpm >= 100 ? digits[bpm / 100] : 0);
}
void Cdj::clearBpm() { for (uint8_t i = 11; i <= 13; ++i) write(3, i, 0); }
bool Cdj::setTempo(int16_t hundredths, bool wide, bool masterTempo) {
    const int16_t limit = wide ? 10000 : 1600;
    if (hundredths < -limit || hundredths > limit) return false;
    uint8_t flags = 0x90 | (wide ? 4 : 0) | (masterTempo ? 8 : 0);
    int16_t value = wide ? hundredths / 10 : hundredths;
    if (value > 0) flags |= 3;
    else if (value < 0) { flags |= 2; value = -value; }
    if ((wide && value < 1000) || (!wide && value >= 1000)) flags |= 0x40;
    else if (!wide) flags |= 0x20;
    if (value >= 1000) value /= 10;
    write(1, 11, flags); write(1, 12, 0);
    write(2, 11, digits[value % 10]);
    write(2, 12, digits[(value % 100) / 10]);
    write(2, 13, wide && value < 100 ? 0 : digits[value / 100]);
    setIndicator(Indicator::Wide, wide);
    setIndicator(Indicator::MasterTempo, masterTempo);
    return true;
}
void Cdj::clearTempo() {
    write(1, 11, 0); write(1, 12, 0);
    for (uint8_t i = 11; i <= 13; ++i) write(2, i, 0);
    setIndicator(Indicator::Wide, false); setIndicator(Indicator::MasterTempo, false);
}
void Cdj::setIndicator(Indicator indicator, bool enabled) {
    uint8_t index, mask;
    switch (indicator) {
        case Indicator::Play: index = 13; mask = 0x20; break;
        case Indicator::Cue: index = 13; mask = 0x10; break;
        case Indicator::Reverse: index = 13; mask = 8; break;
        case Indicator::CueIn: index = 13; mask = 4; break;
        case Indicator::CueOut: index = 13; mask = 0x40; break;
        case Indicator::Beat8: index = 13; mask = 2; break;
        case Indicator::Beat4: index = 13; mask = 1; break;
        case Indicator::Beat2: index = 12; mask = 0x80; break;
        case Indicator::Beat1: index = 12; mask = 0x40; break;
        case Indicator::Wide: index = 12; mask = 4; break;
        case Indicator::MasterTempo: index = 12; mask = 8; break;
        case Indicator::Vinyl: index = 12; mask = 0x10; break;
        case Indicator::QuickReturn: index = 12; mask = 0x20; break;
        case Indicator::Auxiliary7: index = 13; mask = 0x80; break;
        default: return;
    }
    write(5, index, enabled ? commands_[5][index] | mask : commands_[5][index] & ~mask);
}
void Cdj::setJogMode(JogMode mode) {
    const uint8_t value = static_cast<uint8_t>(mode);
    if (value != 0 && (value < 136 || value > 138)) return;
    for (uint8_t g = 0; g < 6; ++g) write(g, 17, value);
}
void Cdj::setDisplayJogPos(uint8_t position) {
    if (position < 1) position = 1;
    if (position > 135) position = 135;
    for (uint8_t g = 0; g < 6; ++g) write(g, 17, position);
}
void Cdj::setDisplaySeparator(LED_STATUS state) {
    write(5, 5, state == on ? commands_[5][5] | 4 : commands_[5][5] & ~4);
}
void Cdj::setProgressPercentage(DIRECTION direction, uint8_t percentage) {
    if (percentage > 100) percentage = 100;
    setProgressSegments(static_cast<uint8_t>(uint16_t(percentage) * 33 / 100), direction);
}
bool Cdj::setProgressSegments(uint8_t segments, DIRECTION direction) {
    if (segments > 33 || (direction != leftToRight && direction != rightToLeft)) return false;
    for (uint8_t j = 0; j < 5; ++j) {
        uint8_t value;
        if (direction == leftToRight)
            value = readAsset(cdj800_detail::barPatterns + uint16_t(segments) * 6 + j);
        else
            value = readAsset(cdj800_detail::barPatterns + 33 * 6 + j) ^
                    readAsset(cdj800_detail::barPatterns + uint16_t(33 - segments) * 6 + j);
        if (j == 0) value |= commands_[5][5] & 7; // markers and separator
        if (j == 4) value |= commands_[5][9] & 0xf0;
        write(5, 5 + j, value);
    }
    return true;
}
bool Cdj::setCueMarkers(const uint8_t *positions, uint8_t count) {
    if (count > 10 || (count != 0 && positions == nullptr)) return false;
    for (uint8_t i = 0; i < count; ++i) if (positions[i] > 33) return false;
    uint8_t packed[5] = {0,0,0,0,0};
    for (uint8_t i = 0; i < count; ++i)
        for (uint8_t j = 0; j < 5; ++j)
            packed[j] |= readAsset(cdj800_detail::markPatterns + uint16_t(positions[i]) * 5 + j);
    packed[4] |= commands_[5][5] & 0xfc;
    for (uint8_t j = 0; j < 5; ++j) write(5, 1 + j, packed[j]);
    return true;
}
bool Cdj::setRawByte(uint8_t group, uint8_t index, uint8_t value) {
    if (group >= 6 || index < 1 || index > 16) return false;
    write(group, index, value); return true;
}
bool Cdj::getFrame(uint8_t group, uint8_t output[20]) const {
    if (group >= 6 || output == nullptr) return false;
    memcpy(output, commands_[group], 20);
    output[0] = group << 4;
    uint8_t sum = 0;
    for (uint8_t i = 1; i <= 17; ++i) sum += output[i];
    output[18] = sum; output[19] = 0;
    return true;
}
