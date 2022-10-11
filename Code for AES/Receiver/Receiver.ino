#include <SPI.h>
#include <RH_RF95.h>
#include <CTR.h>
#include <AES.h>
CTR<AES128> ctr;

//Singleton instance of radio driver
RH_RF95 rf95;
int led = 13;   // Define LED pin in case we want to use it to demonstrate activity


int MESSAGELENGTH = 16;
int DELAY = 1000; // Mean time between transmissions (100 milliseconds)
double CSMATIME = 10;  // Check the status of the channel every 10 ms

uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; 
uint8_t iv[16] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3};

uint8_t plaintext[16];


void setup() {
  pinMode(led, OUTPUT);
  Serial.begin(9600); 
  Serial.println("Receiver Version 1");
  while (!Serial)
    Serial.println("Waiting for serial port");  //Wait for serial port to be available.
  while (!rf95.init())
  {
    Serial.println("Initialisation of LoRa receiver failed");
    delay(1000);
  }
  rf95.setFrequency(915.0);   
  rf95.setTxPower(23, false); 
  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(12);

}

void loop() {
  uint8_t buf[MESSAGELENGTH];
  uint8_t len = sizeof(buf);
  if (rf95.available())
  {
    // Should be a message for us now   
    if (rf95.recv(buf, &len))
    {
      //Serial.print("Received message : "); Serial.println((char*)buf);  //DEBUG
      digitalWrite(led, HIGH);
    }
    else
    {
      Serial.println("recv failed");
    }
  
    // Unpack message
    char str[MESSAGELENGTH];
    for (int i=0; i < MESSAGELENGTH; i++)
    {
      str[i] = buf[i];
      Serial.print(str[i]);
    }
    Serial.println(" ");

    // now decrypt and display text
    ctr.setKey(key, 16);
    ctr.setIV(iv, 16);
  
    ctr.decrypt(plaintext, str, len);

    Serial.println((char*)plaintext);

    ctr.clear();

    
  } 
  digitalWrite(led, LOW);
}
