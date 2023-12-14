#include <Cdj.h>

Cdj::Cdj()
    : status(), sendMessages{
                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                },
      messageHandler(sendMessages)
{
}

void Cdj::init()
{
    messageHandler.init();
}

void Cdj::handleSPI()
{
    byte current = SPDR;
    SPDR = messageHandler.nextByteToSend();
    messageHandler.reciveByte(current);
}

void Cdj::setDisplayJogPos(byte position)
{
    if (position > 135)
    {
        position = 135;
    }

    if (position < 1)
    {
        position = 1;
    }

    for (size_t i = 0; i < 6; i++)
    {
        sendMessages[i][17] = position;
        sendMessages[i][18] = calculateDispMessageCrc(sendMessages[i]);
    }
}

void Cdj::setEjectAnimation()
{
    for (size_t i = 0; i < 6; i++)
    {
        sendMessages[i][17] = 138;
        sendMessages[i][18] = calculateDispMessageCrc(sendMessages[i]);
    }
}

void Cdj::setLoadInAnimation()
{
    for (size_t i = 0; i < 6; i++)
    {
        sendMessages[i][17] = 137;
        sendMessages[i][18] = calculateDispMessageCrc(sendMessages[i]);
    }
}

void Cdj::setDisplayFullJog()
{
    for (size_t i = 0; i < 6; i++)
    {
        sendMessages[i][17] = 136;
        sendMessages[i][18] = calculateDispMessageCrc(sendMessages[i]);
    }
}

void Cdj::clearDisplayJog()
{
    for (size_t i = 0; i < 6; i++)
    {
        sendMessages[i][17] = 0;
        sendMessages[i][18] = calculateDispMessageCrc(sendMessages[i]);
    }
}

StatusMessage Cdj::lastStatus()
{
    if (!messageHandler.isMessageAvailable())
    {
        return status;
    }

    Message message = messageHandler.popMessage();

    if (!StatusMessage::isValidCrc(message.content))
    {
        return status;
    }

    status = StatusMessage::createMessage(message.content);

    return status;
}

byte Cdj::calculateDispMessageCrc(byte (&arr)[BUFFER_LENGTH])
{
    int calculedCrc = 0;

    for (byte ci = 1; ci < BUFFER_LENGTH - 2; ci++)
    {
        calculedCrc += arr[ci];
    }

    calculedCrc = calculedCrc % 255;

    return calculedCrc;
}
