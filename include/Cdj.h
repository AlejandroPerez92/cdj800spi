#ifndef STATUS_CDJ_H
#define STATUS_CDJ_H

#include <Arduino.h>
#include <SPIMessageHandler.h>
#include <StatusMessage.h>

class Cdj
{
private:
    StatusMessage status;
    byte sendMessages[6][BUFFER_LENGTH];
    SPIMessageHandler messageHandler;
    byte calculateDispMessageCrc(byte (&arr)[BUFFER_LENGTH]);
public:
    Cdj();
    void init();
    StatusMessage lastStatus();
    void handleSPI();
    void setDisplayJogPos(byte position);
    void setEjectAnimation();
    void setLoadInAnimation();
    void setDisplayFullJog();
    void clearDisplayJog();
};

#endif