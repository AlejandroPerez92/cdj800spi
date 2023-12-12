#ifndef STATUS_MESSAGE_H
#define STATUS_MESSAGE_H

#include <Arduino.h>
#include <SPIMessageHandler.h>

class StatusMessage
{
public:
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
    bool btnLoop1;
    bool btnLoop2;
    bool btnLoop4;
    bool btnLoop8;
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