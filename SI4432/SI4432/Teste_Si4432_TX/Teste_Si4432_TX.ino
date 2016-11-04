#include "si4432.h"

Si4432 radio(3, 2);
unsigned int pTime;


void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(300);
  radio.init();
  radio.setBaudRate(70);
  radio.setFrequency(433);
 // radio.readAll();

  pTime = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  byte dummy[10] = { 0x01, 0x3, 0x11, 0x13 };
  byte resLen = 0;
  byte answer[64] = { 0 };

  bool pkg = radio.sendPacket(sizeof(dummy), dummy, true, 500, &resLen, answer);


  if (pkg) {

    Serial.print("PACKET CAME - ");
    Serial.println((int) resLen, DEC);

    for (byte i = 0; i < resLen; ++i) {
      Serial.print(answer[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
  } 
  //delay(200);
}
