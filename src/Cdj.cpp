#include <Cdj.h>

Cdj::Cdj()
    : status(), messageHandler()
{
}

void Cdj::init()
{
    messageHandler.init();
}

void Cdj::handleSPI()
{
    byte current = SPDR;
    SPDR = messageHandler.nextByte();
    messageHandler.handleSPI(current);
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
