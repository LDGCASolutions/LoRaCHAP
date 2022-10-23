/*
 * 2022-09-10
  This node assumes it is authenticated and starts broadcasting data
  Data packets are organised as follows
    NODE_ID MESSAGE_TYPE CONTENT 
    
  Server will check the nodeID against stored data to check if it was authenticated recently
  If so, Server will accept the packet
  Else, Server will send a CHAP_FAIL message, triggering the CHAP process on this node
*/
#include <SPI.h>
#include <RH_RF95.h>
#include <SHA256.h>
#include <CTR.h>
#include <Crypto.h>
#include <AES.h>
#include "base64.hpp"

SHA256 sha256;
AES256 aes256; //AES in ECB mode

BlockCipher *cipher = &aes256;

// Singleton instance of the radio driver
RH_RF95 rf95;
float frequency = 915.0;
bool authenticated = true;
int messageIndex = 0;
int i;

char deviceID[10] = "NODE001";
char devicePW[10] = "NODE1PASS";

void hashdigest(Hash *hash, char *plaintext, uint8_t (&digest) [32]) {
  uint8_t value[32];
  
  hash->reset();
  hash->update(plaintext, strlen(plaintext));
  hash->finalize(value, sizeof(value));

  memcpy(digest, value, 32);
}

bool chap() {
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  char sender[10];
  char reqType[10];
  char content[40];

  Serial.println("==============================");
  Serial.println("Authenticating...");
  
  // Request to authenticate by sending the deviceID
  sprintf(buf, "%s %s %s", deviceID, "CHAP_REQ", deviceID);
  rf95.send(buf, sizeof(buf));
  rf95.waitPacketSent();
  
  // Wait for the challenge from the server
  if (rf95.waitAvailableTimeout(10000)) {   
    if (rf95.recv(buf, sizeof(buf))) {
      Serial.print("Message: ");
      Serial.println((char*)buf);
      
      sscanf(buf, "%s %s %s", &sender, &reqType, &content);

      if (strncmp(sender, "GATE", 4) == 0) { // Only a GATE can authenticate
        if (strcmp(reqType, "CHAP_CHAL") == 0) {
          char tmp[40];
          sprintf(tmp, "%s%s", devicePW, content);
          //Serial.println(tmp);
          
          uint8_t digest[32];
          hashdigest(&sha256, tmp, digest);
          Serial.print("CHAP_RESP : ");
          for(i=0; i<32; i++) {
            if (digest[i]<16) Serial.print("0");
            Serial.print(digest[i], HEX);
          }
          Serial.println();
  
          sprintf(buf, "%s %s %s ", deviceID, "CHAP_RESP", digest);
          
          if (rf95.isChannelActive()) {
            Serial.print("Channel busy.");
            while (rf95.isChannelActive()) {
              delay(random(10,100));   
              Serial.print(".");
            }
          }
          
          rf95.send(buf, sizeof(buf));
          rf95.waitPacketSent(); 
  
          if (rf95.waitAvailableTimeout(10000)) {
            if (rf95.recv(buf, sizeof(buf))) {
              sscanf(buf, "%s %s %s", &sender, &reqType, &content);
              
              if ((strcmp(reqType, "CHAP_AUTH") == 0) && strncmp(deviceID, content, 10) == 0) {
                Serial.print(sender);
                Serial.println(" says this node is authenticated! Lets Go!");
                return true;  
              }
            }
          }         
        } else {
          Serial.println("Not CHAP_CHAL");
          Serial.println((char*)buf);
        }  
      }
 
    } else {
      Serial.println("Recv failed");
    }
  }
  else {
    Serial.println("No challenge received");
  }
  
  Serial.println("CHAP Failed");
  return false;
}

void setup() 
{
  Serial.begin(9600);
  //while (!Serial) ; // Wait for serial port to be available
  Serial.println("Start LoRa Client");
  if (!rf95.init())
    Serial.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);

  // Setup Spreading Factor (6 ~ 12)
  rf95.setSpreadingFactor(8);
  
  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  rf95.setSignalBandwidth(125000);
  
  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
  rf95.setCodingRate4(5);
}

void loop() {
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  char sender[10];
  char reqType[10];
  char content[40];
  
  Serial.println("--------------------------------------------");
   
  while (!authenticated) {
    authenticated = chap();
    delay(1000);
  }

  Serial.println("Sending data to LoRa Server");
  
  // Key and initilization vector hard coded 
  byte key[33] = "12345678123456781234567812345678";
  messageIndex++;
  char message[20];
  sprintf(message, "some data %d", messageIndex);
  Serial.println(message);
  
  byte ciphertext[16];
  byte base64_text[20];

  // Encrypt AES-256-ECB
  crypto_feed_watchdog();
  cipher->setKey(key, 32);
  cipher->encryptBlock(ciphertext, message);

  // Base64 encode
  int base64_length = encode_base64(ciphertext,16,base64_text);
  Serial.print("Base64 Text: ");Serial.println((char *) base64_text);
  
  // Send a message to LoRa Server
  sprintf(buf, "%s %s %s", deviceID, "MESSAGE", base64_text);
  if (rf95.isChannelActive()) {
    Serial.print("Channel busy.");
    while (rf95.isChannelActive()) {
      delay(random(10,100));   
      Serial.print(".");
    }
  }
  rf95.send(buf, sizeof(buf));
  rf95.waitPacketSent();

  if (rf95.waitAvailableTimeout(20000)) { // Listen for 10 seconds
    if (rf95.recv(buf, sizeof(buf))) {
      Serial.print("Message: ");
      Serial.println((char*)buf);
      
      sscanf(buf, "%s %s %s", &sender, &reqType, &content);

      if (strcmp(reqType, "CHAP_REQ") == 0) {
        // Another node is trying to authenticate. Give it some privacy
        delay(20000);
      }
  
      if ((strcmp(reqType, "CHAP_FAIL") == 0) && (strncmp(deviceID, content, 10) == 0)) {
        Serial.print(sender);
        Serial.println(" : CHAP_FAIL");
        authenticated = false;
      }
    }
  }
  
  delay(30000);
}
