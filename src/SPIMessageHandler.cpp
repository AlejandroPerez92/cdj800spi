#include "SPIMessageHandler.h"

SPIMessageHandler::SPIMessageHandler()
    : sendMessages{
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
          {80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
      },
      queue{2}, i{0}, currentSendIndex{0}
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

void SPIMessageHandler::handleSPI(byte current)
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
        currentSendIndex++;

        if (currentSendIndex > 5)
        {
            currentSendIndex = 0;
        }

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

byte SPIMessageHandler::nextByte()
{
    if (i < BUFFER_LENGTH - 1)
    {
        return sendMessages[currentSendIndex][i + 1];
    }
    else
    {
        return sendMessages[currentSendIndex + 1][0];
    }
}

Message SPIMessageHandler::popMessage()
{
    return queue.dequeue();
}

bool SPIMessageHandler::isMessageAvailable()
{
    return !queue.isEmpty();
}