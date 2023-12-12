#include <Arduino.h>
#include <Cdj.h>

Cdj cdj;

void setup()
{
  Serial.begin(115200);
  cdj.init();
  Serial.println("Let's go");
}

void loop(void)
{
  StatusMessage parsed = cdj.lastStatus();

  Serial.println(parsed.btnPlay);
  delay(50);
}

ISR(SPI_STC_vect)
{
  cdj.handleSPI();
}