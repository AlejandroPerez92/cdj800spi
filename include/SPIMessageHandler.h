#ifndef SPI_MESSAGE_HANDLER_H
#define SPI_MESSAGE_HANDLER_H

#include <SPI.h>
#include <ArduinoQueue.h>

const size_t BUFFER_LENGTH = 20;

struct Message
{
    byte content[BUFFER_LENGTH];
};

class SPIMessageHandler
{
public:
    SPIMessageHandler();
    void init();
    void handleSPI(byte current);
    byte nextByte();
    bool isMessageAvailable();
    Message popMessage();

private:
    byte sendMessages[6][BUFFER_LENGTH];
    ArduinoQueue<Message> queue;
    byte i;
    byte currentSendIndex;
    byte currentMessage[BUFFER_LENGTH];
};

#endif