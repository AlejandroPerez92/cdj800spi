#ifndef STATUS_MESSAGE_H
#define STATUS_MESSAGE_H

#include <Arduino.h>
#include <SPIMessageHandler.h>

class StatusMessage
{
public:
    StatusMessage();
    static StatusMessage createMessage(byte (&arr)[BUFFER_LENGTH]);
    static bool isValidCrc(byte (&arr)[BUFFER_LENGTH]);
    byte touch;
    uint16_t pitch;
    uint16_t jogPulse;
    uint16_t jogSpeed;
    bool jogActive;
    bool jogForward;
    bool jogPresure;
    bool btnCue;
    bool btnRev;
    bool btnCueOut;
    bool btnCueIn;
    bool btnBeat8;
    bool btnBeat4;
    bool btnBeat2;
    bool btnBeat1;
    bool btnReloop;
    bool btnPlay;
    bool btnSearchFwd;
    bool btnSearchRev;
    bool btnTrackSearchFwd;
    bool btnTrackSearchRev;
    bool btnMasterTempo;
    bool btnTempo;
    bool btnVinyl;
    bool btnEject;
    bool btnQuickReturn;
    bool btnFolderFwd;
    bool btnFolderRev;
    bool btnTimeMode;
    bool btnTextMode;
    bool btnDelete;
    bool btnMemory;
    bool btnCallRev;
    bool btnCallFwd;
};

#endif