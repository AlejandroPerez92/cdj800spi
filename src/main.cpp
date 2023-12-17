#include <Arduino.h>
#include <Cdj.h>

Cdj cdj;

void setup()
{
  Serial.begin(115200);
  cdj.init();
  cdj.setDisplaySeparator(on);
  cdj.setProgressPercentage(50);
  Serial.println("Let's go");
}

void loop(void)
{
  StatusMessage status = cdj.lastStatus();

  if (status.btnBeat4)
  {
    cdj.ledBeat4SetStatus(on);
  }

  if (!status.btnBeat4)
  {
    cdj.ledBeat4SetStatus(off);
  }

  if (status.btnCue)
  {
    cdj.ledCueSetStatus(on);
  }

  if (!status.btnCue)
  {
    cdj.ledCueSetStatus(off);
  }

  delay(10);
}

ISR(SPI_STC_vect)
{
  cdj.handleSPI();
}