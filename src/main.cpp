#include<Arduino.h>
#include<SPI.h>
#include <ArduinoQueue.h>

struct Message {
	byte content[19];
};

ArduinoQueue<Message> messagesQueue(10);

void setup()
{
  pinMode(SCK,INPUT);
  pinMode(MOSI, INPUT);
  pinMode(MISO, OUTPUT);
  pinMode(SS, INPUT);
  digitalWrite(SS, HIGH);

  Serial.begin(115200);

  SPCR = (1 << SPE)|(0 << MSTR);
  SPI.attachInterrupt();
  
  Serial.println("Let's go");
}

void printMessage(Message &message) {

  for (int i = 0; i < 19; i++) {
    Serial.print('|');
    Serial.print(message.content[i]);
  }
  Serial.println();
}

bool isValidMessage(Message &message) {
  if(message.content[0] != 1) {
    return false;
  }

  if(message.content[1] != 16) {
    return false;
  }

  if(message.content[2] != 0) {
    return false;
  }

  if(message.content[6] != 128) {
    return false;
  }

  if(message.content[7] != 192) {
    return false;
  }

  return true;
}

void loop(void)
{
  if (messagesQueue.isEmpty()) {
    return;
  }

  Message message = messagesQueue.dequeue();

  // I can't activate the validation because it crash
  // if(!isValidMessage(message)) {
  //   Serial.println("Invalid data");
  //   return;
  // }

  printMessage(message);
}

byte currentMessage[19];
int i;

ISR (SPI_STC_vect)   //Inerrrput routine function
{
  if(i == 0 && SPDR != 1) {
    // Serial.println("-------");
    i = 0;
    return;
  }

  if(i == 1 && SPDR != 16) {
    // Serial.println("++++++++");
    i = 0;
    return;
  }

  if(i == 2 && SPDR != 0) {
    // Serial.println("++++++++");
    i = 0;
    return;
  }

  currentMessage[i] = SPDR;

  if (i == 18) {
    i = 0;
    if(messagesQueue.isFull()){
      return;
    }

    Message message;
    memcpy(message.content, currentMessage, sizeof(message.content));
    messagesQueue.enqueue(message);
    
    return;
  }

  i ++;
}
