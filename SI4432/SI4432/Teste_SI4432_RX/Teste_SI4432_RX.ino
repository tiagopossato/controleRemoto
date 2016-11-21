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
  //radio.readAll();

  radio.startListening();

  pTime = millis();
}

void loop() {
  // put your main code here, to run repeatedly:
  byte dummy[4] = { 0x55, 0x44, 0x33, 0x11 };

  bool pkg = radio.isPacketReceived();

  if (pkg) {
    byte payLoad[64] = {0};
    byte len = 0;
    radio.getPacketReceived(&len, payLoad);
    Serial.print("PACKET CAME - ");
    Serial.print(len, DEC);
    Serial.print(" - ");
    Serial.println(millis() - pTime, DEC);

    pTime = millis();
    for (byte i = 0; i < len; ++i) {
      Serial.print((int) payLoad[i], HEX);
      Serial.print(" ");
    }
    Serial.println(" ");

    Serial.print("Sending response- ");
    while (!radio.sendPacket(4, dummy));
    Serial.println(" SENT!");
      radio.startListening(); // restart the listening.
  }
  //delay(200);
}
