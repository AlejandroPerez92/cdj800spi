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
    SPIMessageHandler(byte (*messages)[BUFFER_LENGTH]);
    void init();
    void reciveByte(byte current);
    byte nextByteToSend();
    bool isMessageAvailable();
    Message popMessage();

private:
    byte (*sendMessages)[BUFFER_LENGTH];
    ArduinoQueue<Message> queue;
    byte i;
    byte sendIndex;
    byte currentSendMessageIndex;
    byte currentMessage[BUFFER_LENGTH];
};

#endif