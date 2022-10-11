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

// Key and initilization vector hard coded 
uint8_t key[16] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
uint8_t iv[16] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3};

// data to be encrypted
uint8_t plaintext[16] = "x=33.98 y=99.23";
uint8_t ciphertext[16];
size_t len = 16;
  


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
  // Generate message intermittently (10 seconds)
  ctr.setKey(key, 16);
  ctr.setIV(iv, 16);
  ctr.encrypt(ciphertext, plaintext, len);

  
  rf95.setModeIdle(); // some obscure bug causing loss of every second message  
    
  // Channel should be idle but if not wait for it to go idle
  while (rf95.isChannelActive())
  {
    delay(CSMATIME);   // wait for channel to go idle by checking frequently
    Serial.println("Tag node looping on isChannelActive()"); //DEBUG
  }
    
  // Transmit message
  Serial.print("Transmitted message: ");  //DEBUG
  Serial.println((char*)ciphertext);           //DEBUG
  rf95.send(ciphertext, sizeof(ciphertext));
  rf95.waitPacketSent();
  delay(TXINTERVAL);
}
