#pragma once
#include <stddef.h>
#include <stdint.h>

static const size_t BUFFER_LENGTH = 20;

enum class Button : uint8_t {
    Cue, Rev, CueOut, CueIn, Beat8, Beat4, Beat2, Beat1, Reloop, Play,
    SearchFwd, SearchRev, TrackSearchFwd, TrackSearchRev, MasterTempo,
    Tempo, Vinyl, Eject, QuickReturn, FolderFwd, FolderRev, TimeMode,
    TextMode, Delete, Memory, CallRev, CallFwd, Count
};

class StatusMessage {
public:
    StatusMessage();
    static StatusMessage createMessage(const uint8_t arr[BUFFER_LENGTH]);
    static uint8_t checksum(const uint8_t payload[19]);
    static bool isValidCrc(const uint8_t arr[BUFFER_LENGTH]); // legacy name
    static bool isValid(const uint8_t arr[BUFFER_LENGTH]);
    bool down(Button button) const;
    uint32_t buttons() const;

    uint8_t raw[BUFFER_LENGTH];
    uint8_t touch;
    uint16_t pitch, pitchReference, jogPulse, jogSpeed;
    uint8_t jogFlags, auxiliary13;
    bool jogActive, jogForward, jogPresure, jogPressure, jogFlag20;
    bool btnCue, btnRev, btnCueOut, btnCueIn;
    bool btnBeat8, btnBeat4, btnBeat2, btnBeat1, btnReloop, btnPlay;
    bool btnSearchFwd, btnSearchRev, btnTrackSearchFwd, btnTrackSearchRev;
    bool btnMasterTempo, btnTempo, btnVinyl, btnEject, btnQuickReturn;
    bool btnFolderFwd, btnFolderRev, btnTimeMode, btnTextMode;
    bool btnDelete, btnMemory, btnCallRev, btnCallFwd;
};
