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
SHA256 sha256;

// Singleton instance of the radio driver
RH_RF95 rf95;
float frequency = 915.0;
bool authenticated = true;
int i;

char deviceID[10] = "NODE001";
char devicePW[30] = "VerySecurePassword001";

uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
char sender[10];
char reqType[10];
char content[40];

char hashdigest(Hash *hash, char *plaintext) {
  // ThisIsARandomStringVerySecurePassword001
  // 997AD7A742BEAE99A2A67E0EF6835F49728CD2147F2688C7CB0AA7F639E28D7B
  
  uint8_t value[32];
  
  hash->reset();
  hash->update(plaintext, strlen(plaintext));
  hash->finalize(value, sizeof(value));

  Serial.print("Inside hashdigest(): ");
  for(i=0; i<sizeof(value); i++) {
    uint8_t letter[1];
    sprintf(letter,"%x",value[i]);  
    Serial.print((char*)letter);
  }
  Serial.println();

  return value;
}

bool chap() {
  //  Device ID: NODE001
  //  Device PW: VerySecurePassword001

  Serial.println("=========================================================================");
  Serial.println("Authenticating...");
  
  uint8_t digest[32];

  // Request to authenticate by sending the deviceID
  sprintf(buf, "%s %s %s", deviceID, "CHAP_REQ", deviceID);
  rf95.send(buf, sizeof(buf));
  rf95.waitPacketSent();
  
  // Wait for the challenge from the server
  if (rf95.waitAvailableTimeout(3000)) {   
    if (rf95.recv(buf, sizeof(buf))) {
      Serial.print("Message: ");
      Serial.println((char*)buf);
      
      sscanf(buf, "%s %s %s", &sender, &reqType, &content);
      Serial.print("\tSender  : ");
      Serial.println(sender);
      Serial.print("\tRequest : ");
      Serial.println(reqType);
      Serial.print("\tContent : ");
      Serial.println(content);

      if (strcmp(reqType, "CHAP_CHAL") == 0) {
        Serial.print(sender);
        Serial.print(" sent a challenge: ");
        Serial.println(content);
        
        sprintf(buf, "%s%s", content, devicePW);

        Serial.print("Challenge response (Before hashing): ");
        Serial.println((char*)buf);

        digest[32] = hashdigest(&sha256, buf);

        // ThisIsARandomStringVerySecurePassword001
        // 997AD7A742BEAE99A2A67E0EF6835F49728CD2147F2688C7CB0AA7F639E28D7B <- Hopefully digest is this

        sprintf(buf, "%s %s %s", deviceID, "CHAP_RESP", digest);
        Serial.println("Sending challenge response");
        
        rf95.send(buf, sizeof(buf));
        rf95.waitPacketSent(); 

        if (rf95.waitAvailableTimeout(3000)) {   
          if (rf95.recv(buf, sizeof(buf))) {
            sscanf(buf, "%s %s %s", &sender, &reqType, &content);
            Serial.print("\tSender  : ");
            Serial.println(sender);
            Serial.print("\tRequest : ");
            Serial.println(reqType);
            Serial.print("\tContent : ");
            Serial.println(content);
            
            if ((strcmp(reqType, "CHAP_AUTH") == 0) && strncmp(deviceID, content, 10) == 0) {
              Serial.print(sender);
              Serial.println(" says this node is authenticated! Lets Go!");
              return true;  
            }
          }
        }         
      } else {
        Serial.println("ELSE");
        Serial.println((char*)buf);
      }

//      Serial.print("Challenge response: ");
//      for(i=0; i<sizeof(digest); i++) {
//        uint8_t letter[1];
//        sprintf(letter,"%x",digest[i]);  
//        Serial.print((char*)letter);
//      }
//      Serial.println();
//      rf95.send(digest, sizeof(digest));
 
    } else {
      Serial.println("Challenge recv failed");
    }
  }
  else {
    Serial.println("No challenge");
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
  rf95.setSpreadingFactor(7);
  
  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  //Lower BandWidth for longer distance.
  rf95.setSignalBandwidth(125000);
  
  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
  rf95.setCodingRate4(5);
}

void loop() {
  Serial.println("-------------------------------------------------------------------------");
   
  while (!authenticated) {
    authenticated = chap();
    delay(15000);
  }

  Serial.println("Sending data to LoRa Server");
  // Send a message to LoRa Server
  sprintf(buf, "%s %s %s", deviceID, "MESSAGE", "Heres some data :-P");
  rf95.send(buf, sizeof(buf));
  rf95.waitPacketSent();

  if (rf95.waitAvailableTimeout(3000)) {
    if (rf95.recv(buf, sizeof(buf))) {
  //    Serial.print("Message: ");
  //    Serial.println((char*)buf);
      
      sscanf(buf, "%s %s %s", &sender, &reqType, &content);
      Serial.print("\tSender  : ");
      Serial.println(sender);
      Serial.print("\tRequest : ");
      Serial.println(reqType);
      Serial.print("\tContent : ");
      Serial.println(content);
  
      if ((strcmp(reqType, "CHAP_FAIL") == 0) && strncmp(deviceID, content, 10) == 0) {
        Serial.print(sender);
        Serial.println(" says this node is not authenticated!");
        authenticated = false;
      }
    }
  }
  
  delay(10000);
}
