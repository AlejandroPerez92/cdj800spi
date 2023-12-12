#include <Arduino.h>
#include <SPIMessageHandler.h>
#include <StatusMessage.h>

SPIMessageHandler spi;

void setup()
{
  Serial.begin(115200);
  spi.init();
  Serial.println("Let's go");
}

void loop(void)
{
  if (!spi.isMessageAvailable())
  {
    return;
  }

  Message message = spi.popMessage();
  if (!StatusMessage::isValidCrc(message.content))
  {
    return;
  }

  StatusMessage parsed = StatusMessage::createMessage(message.content);

  Serial.println(parsed.btnPlay);
  delay(50);
}

ISR(SPI_STC_vect)
{
  byte current = SPDR;
  SPDR = spi.nextByte();
  spi.handleSPI(current);
}
