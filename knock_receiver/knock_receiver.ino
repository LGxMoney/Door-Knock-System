#include <RH_ASK.h>
#include <SPI.h> // Not actualy used but needed to compile
#include "pitches.h"

RH_ASK driver;
int Index;

int stepPin = 6;
int enablePin = 7;
int directionPin = 12;


void setup()
{
    Serial.begin(9600); // Debugging only
    if (!driver.init())
         Serial.println("init failed");

    //motor set up
    pinMode(enablePin, OUTPUT); //Enable
    pinMode(stepPin, OUTPUT); //Step
    pinMode(directionPin, OUTPUT); //Direction
  
    Serial.println("Starting up reciever");
    
    pullDown();
    playSound();
    delay(1000);
    reset();
}

void loop()
{
    uint8_t buf[12];
    uint8_t buflen = sizeof(buf);
    if (driver.recv(buf, &buflen)) // Non-blocking
    {
      Serial.println("It worked");
      int i;
      
      pullDown();
      playSound();
      delay(1000);
      reset();
      Serial.println((char*)buf);     
    }
}

void pullDown()
{
  digitalWrite(enablePin, LOW);
  digitalWrite(directionPin, LOW);
  for(Index = 0; Index < 1200; Index++)
  {
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
  }
}

void reset()
{
  digitalWrite(directionPin, HIGH);
  for(Index = 0; Index < 1200; Index++) 
  {
    digitalWrite(stepPin,LOW);
    delayMicroseconds(500);
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500);
  }
  digitalWrite(stepPin, LOW);
  digitalWrite(enablePin, HIGH);
  
}

void playSound()
{
    tone(4,NOTE_E6,125);
  delay(130);
  tone(4,NOTE_G6,125);
  delay(130);
  tone(4,NOTE_E7,125);
  delay(130);
  tone(4,NOTE_C7,125);
  delay(130);
  tone(4,NOTE_D7,125);
  delay(130);
  tone(4,NOTE_G7,125);
  delay(125);
  noTone(4);
}
