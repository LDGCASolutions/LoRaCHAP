#include <Console.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <CTR.h>
#include <AES.h>
CTR<AES128> ctr;

//Singleton instance of radio driver
RH_RF95 rf95;
int led = 13;   // Define LED pin in case we want to use it to demonstrate activity

int MESSAGELENGTH = 16;

void setup() {
  Bridge.begin(115200);
  Console.begin();
  Console.println("Gateway Version 3");
  while (!Console) Console.println("Waiting for Console port");  //Wait for Console port to be available.
  while (!rf95.init()) {
    Console.println("Initialisation of LoRa receiver failed");
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
      Console.println("Received message : ");
      for(int i=0; i<16; i++) {
        Console.print(buf[i]);
        Console.print(" " );
      }
      Console.println();

      uint8_t key[16] = "NODE1PASS";
      uint8_t iv[16] = {0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3};
      uint8_t plaintext[16];
      
      ctr.setKey(key, 16);
      ctr.setIV(iv, 16);
      ctr.decrypt(plaintext, buf, 16);
      
      for(int i=0; i<16; i++) {
        Console.print(plaintext[i]);
        Console.print(" " );
      }
      Console.println();
      
      for(int i=0; i<16; i++) {
        Console.print((char)plaintext[i]);
        Console.print(" " );
      }
      Console.println();
      Console.println("----------------------------------");
      ctr.clear(); 
    }
  }
}
