#include <Cdj.h>

Cdj::Cdj()
    : status(), commands{
                    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {32, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {48, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                    {80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                },
      messageHandler(commands)
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
        commands[i][17] = position;
        calculateCommandCrc(i);
    }
}

void Cdj::setEjectAnimation()
{
    for (size_t i = 0; i < 6; i++)
    {
        commands[i][17] = 138;
        calculateCommandCrc(i);
    }
}

void Cdj::setLoadInAnimation()
{
    for (size_t i = 0; i < 6; i++)
    {
        commands[i][17] = 137;
        calculateCommandCrc(i);
    }
}

void Cdj::setDisplayFullJog()
{
    for (int i = 0; i < 6; i++)
    {
        commands[i][17] = 136;
        calculateCommandCrc(i);
    }
}

void Cdj::clearDisplayJog()
{
    for (size_t i = 0; i < 6; i++)
    {
        commands[i][17] = 0;
        calculateCommandCrc(i);
    }
}

void Cdj::setDisplaySeparator(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][5] |= (1 << 2);
    }

    if (status == off)
    {
        commands[buttonStatus][5] &= ~(1 << 2);
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::setProgressPercentage(DIRECTION direction, byte percentage)
{
    if (percentage > 100)
    {
        percentage = 100;
    }

    if (percentage < 0)
    {
        percentage = 0;
    }

    byte result = (percentage * 37) / 100;

    if (leftToRight == direction)
    {
        for (size_t i = 0; i < result; i++)
        {
            if (i < 8)
            {
                commands[buttonStatus][9] |= (1 << (7 - i));
                continue;
            }

            if (i < 16)
            {
                commands[buttonStatus][8] |= (1 << (15 - i));
                continue;
            }

            if (i < 24)
            {
                commands[buttonStatus][7] |= (1 << (23 - i));
                continue;
            }

            if (i < 32)
            {
                commands[buttonStatus][6] |= (1 << (31 - i));
                continue;
            }

            commands[buttonStatus][5] |= (1 << (39 - i));
        }
    }
    else
    {
        for (size_t i = 0; i < result; i++)
        {
            if (i < 6)
            {
                commands[buttonStatus][5] |= (1 << (i + 3));
                continue;
            }

            if (i < 14)
            {
                commands[buttonStatus][6] |= (1 << (i - 6));
                continue;
            }

            if (i < 22)
            {
                commands[buttonStatus][7] |= (1 << (i - 14));
                continue;
            }

            if (i < 30)
            {
                commands[buttonStatus][8] |= (1 << (i - 22));
                continue;
            }

            commands[buttonStatus][9] |= (1 << (i - 30));
        }
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::ledPlaySetStatus(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][13] |= (1 << 5);
    }

    if (status == off)
    {
        commands[buttonStatus][13] &= ~(1 << 5);
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::ledCueSetStatus(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][13] |= (1 << 4);
    }

    if (status == off)
    {
        commands[buttonStatus][13] &= ~(1 << 4);
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::ledRevSetStatus(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][13] |= (1 << 3);
    }

    if (status == off)
    {
        commands[buttonStatus][13] &= ~(1 << 3);
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::ledCueInSetStatus(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][13] |= (1 << 2);
    }

    if (status == off)
    {
        commands[buttonStatus][13] &= ~(1 << 2);
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::ledCueOutSetStatus(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][13] |= (1 << 6);
    }

    if (status == off)
    {
        commands[buttonStatus][13] &= ~(1 << 6);
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::ledBeat8SetStatus(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][13] |= (1 << 1);
    }

    if (status == off)
    {
        commands[buttonStatus][13] &= ~(1 << 1);
    }

    calculateCommandCrc(buttonStatus);
}

void Cdj::ledBeat4SetStatus(LED_STATUS status)
{
    if (status == on)
    {
        commands[buttonStatus][13] |= (1 << 0);
    }

    if (status == off)
    {
        commands[buttonStatus][13] &= ~(1 << 0);
    }

    calculateCommandCrc(buttonStatus);
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

void Cdj::calculateCommandCrc(int i)
{
    int calculedCrc = 0;

    for (byte ci = 1; ci < BUFFER_LENGTH - 2; ci++)
    {
        calculedCrc += commands[i][ci];
    }

    calculedCrc = calculedCrc % 256;
    commands[i][18] = (byte)calculedCrc;
}
