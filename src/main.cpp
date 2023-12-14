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
  StatusMessage status = cdj.lastStatus();
  
  if(status.btnPlay) {
    cdj.clearDisplayJog();
  }

  if(status.btnEject) {
    cdj.setLoadInAnimation();
  }

  Serial.println(status.btnPlay);
  delay(100);
}

ISR(SPI_STC_vect)
{
  cdj.handleSPI();
}