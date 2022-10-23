#include <CTR.h>
#include <AES.h>
#include <SPI.h>
#include <RH_RF95.h>
CTR<AES128> ctr; //AES in CTR mode

//Singleton instance of radio driver
RH_RF95 rf95;
int led = 13;   // Define LED pin in case we want to use it to demonstrate activity

int MESSAGELENGTH = 16;
int TXINTERVAL = 5000; // Time between transmissions (in ms)
double CSMATIME = 10;  // Check the status of the channel every 10 ms

void setup() {
  // Initialise LoRa transceiver
  pinMode(led, OUTPUT);
  Serial.begin(9600);   

  Serial.println("Tag version 1");
  while (!Serial)
    Serial.println("Waiting for serial port");  //Wait for serial port to be available.
  while (!rf95.init()){
    Serial.println("Initialisation of LoRa receiver failed");
    delay(1000);
  }
  rf95.setFrequency(915.0);   //PB set to use 915 MHz
  rf95.setTxPower(5, false);
  rf95.setSignalBandwidth(500000);
  rf95.setSpreadingFactor(12);
}

void loop() {
  // Key and initilization vector hard coded 
  uint8_t key[16] = "NODE1PASS";
  uint8_t iv[16] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3};
  
  // data to be encrypted
  uint8_t plaintext[16] = "secret_message";
  uint8_t ciphertext[16];

  Serial.println("Plaintext");
  Serial.println((char*)plaintext);

  ctr.setKey(key, 16);
  ctr.setIV(iv, 16);
  ctr.encrypt(ciphertext, plaintext, 16);

  Serial.println("Ciphertext");
  for(int i=0; i<16; i++) {
    Serial.print(ciphertext[i]);
    Serial.print(" ");
  }
  Serial.println();
  Serial.println("-------------------------");

  rf95.send(ciphertext, 16);
  rf95.waitPacketSent();
  delay(1000);
}
