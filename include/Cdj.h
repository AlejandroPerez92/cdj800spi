#ifndef STATUS_CDJ_H
#define STATUS_CDJ_H

#include <Arduino.h>
#include <SPIMessageHandler.h>
#include <StatusMessage.h>

class Cdj
{
private:
    StatusMessage status;
    SPIMessageHandler messageHandler;
public:
    Cdj();
    void init();
    StatusMessage lastStatus();
    void handleSPI();
};

#endif