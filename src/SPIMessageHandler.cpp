#include "SPIMessageHandler.h"

SPIMessageHandler::SPIMessageHandler(byte (*messages)[BUFFER_LENGTH])
    : sendMessages(messages),
      queue{2}, i{0}, sendIndex{0}, currentSendMessageIndex{0}
{
}

void SPIMessageHandler::init()
{
    pinMode(SCK, INPUT);
    pinMode(MOSI, INPUT);
    pinMode(MISO, OUTPUT);
    pinMode(SS, INPUT);
    digitalWrite(SS, HIGH);

    Serial.begin(115200);

    SPCR = (1 << SPE) | (0 << MSTR);
    SPI.setDataMode(SPI_MODE3);
    SPI.attachInterrupt();
}

void SPIMessageHandler::reciveByte(byte current)
{
    // Find the start of the message
    if (i == 0 && current != 1)
    {
        i = 0;
        return;
    }

    if (i == 1 && current != 16)
    {
        i = 0;
        return;
    }

    currentMessage[i] = current;
    i++;

    // Action on a complete message
    if (i == BUFFER_LENGTH)
    {
        i = 0;

        if (queue.isFull())
        {
            queue.dequeue();
        }

        Message message;
        memcpy(message.content, currentMessage, BUFFER_LENGTH);
        queue.enqueue(message);
        return;
    }
}

byte SPIMessageHandler::nextByteToSend()
{
    sendIndex ++;

    if (sendIndex == BUFFER_LENGTH)
    {
        currentSendMessageIndex ++;
        sendIndex = 0;
    }

    if (currentSendMessageIndex > 5)
    {
        currentSendMessageIndex = 0;
    }
    
    return sendMessages[currentSendMessageIndex][sendIndex];
}

Message SPIMessageHandler::popMessage()
{
    return queue.dequeue();
}

bool SPIMessageHandler::isMessageAvailable()
{
    return !queue.isEmpty();
}