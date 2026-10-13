#include "StatusMessage.h"
#include <string.h>

namespace {
const uint8_t buttonBytes[] = {
    14,14,14,14,14,14,14,14,15,15,16,16,16,16,17,17,17,17,17,17,17,18,18,18,18,18,18
};
const uint8_t buttonMasks[] = {
    0x80,0x20,0x40,0x10,8,4,2,1,2,1,8,4,2,1,0x40,0x20,0x10,8,4,2,1,0x20,0x10,8,4,2,1
};
uint16_t be16(const uint8_t *p) {
    return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
}
}

StatusMessage::StatusMessage() : raw{}, touch(0), pitch(0), pitchReference(0),
    jogPulse(0), jogSpeed(0), jogFlags(0), auxiliary13(0), jogActive(false),
    jogForward(false), jogPresure(false), jogPressure(false), jogFlag20(false),
    btnCue(false), btnRev(false), btnCueOut(false), btnCueIn(false),
    btnBeat8(false), btnBeat4(false), btnBeat2(false), btnBeat1(false),
    btnReloop(false), btnPlay(false), btnSearchFwd(false), btnSearchRev(false),
    btnTrackSearchFwd(false), btnTrackSearchRev(false), btnMasterTempo(false),
    btnTempo(false), btnVinyl(false), btnEject(false), btnQuickReturn(false),
    btnFolderFwd(false), btnFolderRev(false), btnTimeMode(false), btnTextMode(false),
    btnDelete(false), btnMemory(false), btnCallRev(false), btnCallFwd(false) {}

uint8_t StatusMessage::checksum(const uint8_t payload[19]) {
    uint16_t sum = 0;
    for (uint8_t i = 0; i < 19; ++i) {
        sum += payload[i];
        sum = (sum & 255u) + (sum >> 8);
    }
    return static_cast<uint8_t>(sum);
}
bool StatusMessage::isValidCrc(const uint8_t arr[BUFFER_LENGTH]) {
    return checksum(arr) == arr[19];
}
bool StatusMessage::isValid(const uint8_t arr[BUFFER_LENGTH]) {
    return arr[0] == 1 && arr[1] == 0x10 && isValidCrc(arr);
}
bool StatusMessage::down(Button button) const {
    const uint8_t index = static_cast<uint8_t>(button);
    return index < static_cast<uint8_t>(Button::Count) &&
           (raw[buttonBytes[index]] & buttonMasks[index]) != 0;
}
uint32_t StatusMessage::buttons() const {
    uint32_t mask = 0;
    for (uint8_t i = 0; i < static_cast<uint8_t>(Button::Count); ++i)
        if (down(static_cast<Button>(i))) mask |= uint32_t(1) << i;
    return mask;
}
StatusMessage StatusMessage::createMessage(const uint8_t arr[BUFFER_LENGTH]) {
    StatusMessage m;
    memcpy(m.raw, arr, BUFFER_LENGTH);
    m.touch = arr[3]; m.pitch = be16(arr + 4); m.pitchReference = be16(arr + 6);
    m.jogPulse = be16(arr + 8); m.jogSpeed = be16(arr + 10);
    m.jogFlags = arr[12]; m.auxiliary13 = arr[13];
    m.jogActive = (arr[12] & 0x80) != 0;
    m.jogForward = (arr[12] & 0x40) != 0;
    m.jogFlag20 = (arr[12] & 0x20) != 0;
    m.jogPresure = m.jogPressure = (arr[12] & 0x10) != 0;
    m.btnCue = m.down(Button::Cue); m.btnRev = m.down(Button::Rev);
    m.btnCueOut = m.down(Button::CueOut); m.btnCueIn = m.down(Button::CueIn);
    m.btnBeat8 = m.down(Button::Beat8); m.btnBeat4 = m.down(Button::Beat4);
    m.btnBeat2 = m.down(Button::Beat2); m.btnBeat1 = m.down(Button::Beat1);
    m.btnReloop = m.down(Button::Reloop); m.btnPlay = m.down(Button::Play);
    m.btnSearchFwd = m.down(Button::SearchFwd); m.btnSearchRev = m.down(Button::SearchRev);
    m.btnTrackSearchFwd = m.down(Button::TrackSearchFwd);
    m.btnTrackSearchRev = m.down(Button::TrackSearchRev);
    m.btnMasterTempo = m.down(Button::MasterTempo); m.btnTempo = m.down(Button::Tempo);
    m.btnVinyl = m.down(Button::Vinyl); m.btnEject = m.down(Button::Eject);
    m.btnQuickReturn = m.down(Button::QuickReturn);
    m.btnFolderFwd = m.down(Button::FolderFwd); m.btnFolderRev = m.down(Button::FolderRev);
    m.btnTimeMode = m.down(Button::TimeMode); m.btnTextMode = m.down(Button::TextMode);
    m.btnDelete = m.down(Button::Delete); m.btnMemory = m.down(Button::Memory);
    m.btnCallRev = m.down(Button::CallRev); m.btnCallFwd = m.down(Button::CallFwd);
    return m;
}
