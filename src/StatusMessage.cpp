#include <StatusMessage.h>

StatusMessage StatusMessage::createMessage(byte (&arr)[BUFFER_LENGTH])
{
    StatusMessage message;
    message.touch = arr[3];
    message.pitch = (static_cast<uint16_t>(arr[4]) << 8) | arr[5];
    message.jogPulse = (static_cast<uint16_t>(arr[8]) << 8) | arr[9];
    message.jogSpeed = (static_cast<uint16_t>(arr[10]) << 8) | arr[11];
    message.jogActive = arr[12] & 0x80;
    message.jogForward = arr[12] & 0x40;
    message.jogPresure = arr[12] & 0x10;
    message.btnCue = arr[14] & 0x80;
    message.btnRev = arr[14] & 0x20;
    message.btnCueOut = arr[14] & 0x40;
    message.btnCueIn = arr[14] & 0x10;
    message.btnLoop1 = arr[14] & 0x08;
    message.btnLoop2 = arr[14] & 0x04;
    message.btnLoop4 = arr[14] & 0x02;
    message.btnLoop8 = arr[14] & 0x01;
    message.btnReloop = arr[15] & 0x02;
    message.btnPlay = arr[15] & 0x01;
    message.btnSearchFwd = arr[16] & 0x08;
    message.btnSearchRev = arr[16] & 0x04;
    message.btnTrackSearchFwd = arr[16] & 0x02;
    message.btnTrackSearchRev = arr[16] & 0x01;
    message.btnMasterTempo = arr[17] & 0x40;
    message.btnTempo = arr[17] & 0x20;
    message.btnVinyl = arr[17] & 0x10;
    message.btnEject = arr[17] & 0x08;
    message.btnQuickReturn = arr[17] & 0x04;
    message.btnFolderFwd = arr[17] & 0x02;
    message.btnFolderRev = arr[17] & 0x01;
    message.btnTimeMode = arr[18] & 0x20;
    message.btnTextMode = arr[18] & 0x10;
    message.btnDelete = arr[18] & 0x08;
    message.btnMemory = arr[18] & 0x04;
    message.btnCallRev = arr[18] & 0x02;
    message.btnCallFwd = arr[18] & 0x01;
    return message;
}

bool StatusMessage::isValidCrc(byte (&arr)[BUFFER_LENGTH])
{
    int calculedCrc = 0;

    for (byte ci = 0; ci < BUFFER_LENGTH - 1; ci++)
    {
        calculedCrc += arr[ci];
    }

    if ((calculedCrc % 255) != arr[BUFFER_LENGTH - 1])
    {
        return false;
    }

    return true;
}