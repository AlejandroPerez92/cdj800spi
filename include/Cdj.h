#ifndef STATUS_CDJ_H
#define STATUS_CDJ_H

#include <Arduino.h>
#include <SPIMessageHandler.h>
#include <StatusMessage.h>

enum LED_STATUS
{
    on,
    off,
};

enum COMMAND_TYPE
{
    buttonStatus = 5,
};

enum DIRECTION
{
    rightToLeft,
    leftToRight,
};

class Cdj
{
private:
    StatusMessage status;
    byte commands[6][BUFFER_LENGTH];
    SPIMessageHandler messageHandler;
    void calculateCommandCrc(int i);
public:
    Cdj();
    void init();
    StatusMessage lastStatus();
    void handleSPI();
    void setDisplayJogPos(byte position);
    void setEjectAnimation();
    void setLoadInAnimation();
    void setDisplayFullJog();
    void setDisplaySeparator(LED_STATUS status);
    void setProgressPercentage(DIRECTION direction,byte percentage);
    void clearDisplayJog();
    void ledPlaySetStatus(LED_STATUS status);
    void ledCueSetStatus(LED_STATUS status);
    void ledRevSetStatus(LED_STATUS status);
    void ledCueInSetStatus(LED_STATUS status);
    void ledCueOutSetStatus(LED_STATUS status);
    void ledBeat8SetStatus(LED_STATUS status);
    void ledBeat4SetStatus(LED_STATUS status);
};

#endif