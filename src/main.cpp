#include<Arduino.h>
#include<SPI.h>
#include <ArduinoQueue.h>

const int LENGTH = 20;
struct Message {
  byte touch;
  uint16_t pitch;
  uint16_t jogPulse;
  uint16_t jogSpeed;
  bool jogActive;
  bool jogForward;
  bool jogPresure;
  bool btnCue;
  bool btnRev;
  bool btnCueOut;
  bool btnCueIn;
  bool btnLoop1;
  bool btnLoop2;
  bool btnLoop4;
  bool btnLoop8;
  bool btnReloop;
  bool btnPlay;
  bool btnSearchFwd;
  bool btnSearchRev;
  bool btnTrackSearchFwd;
  bool btnTrackSearchRev;
  bool btnMasterTempo;
  bool btnTempo;
  bool btnVinyl;
  bool btnEject;
  bool btnQuickReturn;
  bool btnFolderFwd;
  bool btnFolderRev;
  bool btnTimeMode;
  bool btnTextMode;
  bool btnDelete;
  bool btnMemory;
  bool btnCallRev;
  bool btnCallFwd;
};

ArduinoQueue<Message> messagesQueue(2);

void setup()
{
  pinMode(SCK,INPUT);
  pinMode(MOSI, INPUT);
  pinMode(MISO, OUTPUT);
  pinMode(SS, INPUT);
  digitalWrite(SS, HIGH);

  Serial.begin(115200);

  SPCR = (1 << SPE)|(0 << MSTR);
  SPI.setDataMode(SPI_MODE3);
  SPI.attachInterrupt();
  
  Serial.println("Let's go");
}

void loop(void)
{
  if (messagesQueue.isEmpty()) {
    return;
  }

  Message message = messagesQueue.dequeue();
  
  Serial.println(message.jogSpeed);
  delay(50);
}

Message createMessage(byte (&arr)[LENGTH])
{
  Message message;
  message.touch = arr[3];
  message.pitch = (static_cast<uint16_t>(arr[4]) << 8) | arr[5];
  message.jogPulse = (static_cast<uint16_t>(arr[8]) << 8) | arr[9];
  message.jogSpeed = (static_cast<uint16_t>(arr[10]) << 8) | arr[11];
  message.jogActive = arr[12] & 0x80;
  message.jogForward = arr[12] & 0x40;
  message.jogPresure = arr[12] & 0x10;
  message.btnCue = arr[14] & 0x80;
  message.btnRev = arr[14] & 0x20; 
  message.btnCueOut = arr[14] & 0x40;
  message.btnCueIn = arr[14] & 0x10;
  message.btnLoop1 = arr[14] & 0x08; 
  message.btnLoop2 = arr[14] & 0x04; 
  message.btnLoop4 = arr[14] & 0x02; 
  message.btnLoop8 = arr[14] & 0x01;
  message.btnReloop = arr[15] & 0x02; 
  message.btnPlay = arr[15] & 0x01;
  message.btnSearchFwd = arr[16] & 0x08;
  message.btnSearchRev = arr[16] & 0x04;
  message.btnTrackSearchFwd = arr[16] & 0x02;
  message.btnTrackSearchRev = arr[16] & 0x01;
  message.btnMasterTempo = arr[17] & 0x40;
  message.btnTempo = arr[17] & 0x20;
  message.btnVinyl = arr[17] & 0x10;
  message.btnEject = arr[17] & 0x08;
  message.btnQuickReturn = arr[17] & 0x04;
  message.btnFolderFwd = arr[17] & 0x02;
  message.btnFolderRev = arr[17] & 0x01;
  message.btnTimeMode = arr[18] & 0x20;
  message.btnTextMode = arr[18] & 0x10;
  message.btnDelete = arr[18] & 0x08;
  message.btnMemory = arr[18] & 0x04;
  message.btnCallRev = arr[18] & 0x02;
  message.btnCallFwd = arr[18] & 0x01;
  return message;
}

byte currentMessage[LENGTH];
int i = 0;

//Inerrrput routine function
ISR (SPI_STC_vect)
{
  byte current = SPDR;
  SPDR = 0;

  //Check integrity message integrity
  if(i == 0 && current != 1) {
    i = 0;
    return;
  }

  if(i == 1 && current != 16) {
    i = 0;
    return;
  }

  currentMessage[i] = current;
  i ++;

  //Action on a complete message
  if (i == LENGTH) {
    i = 0;
    
    //CheckCrc
    int calculedCrc = 0;

    for (byte ci = 0; ci < LENGTH - 1; ci++)
    {
      calculedCrc += currentMessage[ci];
    }
    
    if((calculedCrc % 255) != currentMessage[LENGTH - 1]){
      return;
    }

    if(messagesQueue.isFull()){
      messagesQueue.dequeue();
    }

    Message message = createMessage(currentMessage);
    messagesQueue.enqueue(message);

    return;
  }
}
