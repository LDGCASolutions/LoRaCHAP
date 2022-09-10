/*
 * 2022-09-10
  Server listens to broadcasts.
  If sender is authenticated, message will be processed.
  Else, the sender will be asked to repeat CHAP
*/

#define BAUDRATE 115200
#include <Console.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <SHA256.h>
SHA256 sha256;

// Singleton instance of the radio driver
RH_RF95 rf95;

int led = A2;
float frequency = 915.0;

char deviceID[10] = "GATE001";
//uint8_t nodeDigest[32];

// The following device data shuould be retrieved from a DB

char devices[10][10] = {"NODE001", "NODE002"};
char passwords[10][30] = {"VerySecurePassword001", "PW2"};
char challenges[10][30] = {"Chall001", "Chall002"};
bool authenticated[10] = {};

char devicePW[] = "VerySecurePassword001";

char genChallenge() {
  return "ThisIsARandomString";
}

bool authStat(char *deviceID) {
  // Check the authentication status of the device
  Serial.println(deviceID);
  for (int i; i<10; i++) {
    if (strncmp(*devices[i], *deviceID, 10) == 0) {
      if (authenticated[i]) {
        return true;
      }
    }  
  }
  return false;
}

void authSet(char *sender) {
  // Check the authentication status of the device
  for (int i; i<10; i++) {
    if (strncmp(*devices[i], *sender, 10) == 0) {
      authenticated[i] = true;
    }  
  }
}

uint8_t hashdigest(Hash *hash, char *plaintext) {  
  uint8_t value[32];

  hash->reset();
  hash->update(plaintext, strlen(plaintext));
  hash->finalize(value, sizeof(value));

//  Console.print("Inside hashdigest(): ");
//  for(int i; i<sizeof(value); i++) {
//    uint8_t letter[1];
//    sprintf(letter,"%x",value[i]);  
//    Console.print((char*)letter);
//  }
//  Console.println();

  return value;
}

void setup() 
{
  pinMode(led, OUTPUT);     
  Bridge.begin(BAUDRATE);
  Console.begin();
  while (!Console); // Wait for console port to be available
  Console.println("Start Sketch");
  if (!rf95.init())
    Console.println("init failed");
  // Setup ISM frequency
  rf95.setFrequency(frequency);
  // Setup Power,dBm
  rf95.setTxPower(13);
  
  // Setup Spreading Factor (6 ~ 12)
  rf95.setSpreadingFactor(7);
  
  // Setup BandWidth, option: 7800,10400,15600,20800,31200,41700,62500,125000,250000,500000
  rf95.setSignalBandwidth(125000);
  
  // Setup Coding Rate:5(4/5),6(4/6),7(4/7),8(4/8) 
  rf95.setCodingRate4(5);
  
  Console.print("Listening on frequency: ");
  Console.println(frequency);
}

void loop() {
  if (rf95.available()) {
    // Should be a message for us now   
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    char challenge[30] = "ThisIsARandomString"; // <== NEED TO ASSIGN THIS FROM A FUNCTION

    if (rf95.waitAvailableTimeout(3000)) {
      if (rf95.recv(buf, sizeof(buf))) {
        digitalWrite(led, HIGH);
        Console.println("=========================================================================");
        Console.print("Message: ");
        Console.println((char*)buf);
              
        char sender[10];
        char reqType[10];
        char content[40];
        
        sscanf(buf, "%s %s %s", &sender, &reqType, &content);

        if (strcmp(reqType, "MESSAGE") == 0) {
          // Check the authentication status of the device
          if (authStat(sender)) {
            // Device authenticated. Accept the message.
            Console.print(sender);
            Console.println(" is authenticated");

            // What happens next depends on what this device is
            
          } else {
            Console.print(sender);
            Console.println(" is NOT authenticated");
  
            sprintf(buf, "%s %s %s", deviceID, "CHAP_FAIL", sender);
            rf95.send(buf, sizeof(buf));
            rf95.waitPacketSent();
            Console.println("CHAP reset request sent");
          }
        }

        if (strcmp(reqType, "CHAP_REQ") == 0) {
          Console.print(content);
          Console.println(" is trying to authenticate");
          sprintf(buf, "%s %s %s", deviceID, "CHAP_CHAL", challenge);
          rf95.send(buf, sizeof(buf));
          rf95.waitPacketSent();
          Console.println("Challenge sent");
        }

        if (strcmp(reqType, "CHAP_RESP") == 0) {
          Console.print("Challenge response recieved from ");
          Console.println(sender);
  
          uint8_t nodeDigest[32];
          sscanf(buf, "%s %s %s", &sender, &reqType, &nodeDigest);

          Console.print("Recieved hash: ");
          for(int i; i<strlen((char*)nodeDigest); i++) {
            uint8_t letter[1];
            sprintf(letter,"%x",nodeDigest[i]);  
            Console.print((char*)letter);
          }
          Console.println();

          Console.print("Recieved hash string length: ");
          Console.println(strlen((char*)nodeDigest));
  
          uint8_t temp1[50];
          sprintf(temp1, "%s%s", challenge, devicePW);
//          Console.println((char*)temp1);
  
          uint8_t digest[32];
          sscanf(digest, "%s", hashdigest(&sha256, temp1));
  
          // 997ad7a742beae99a2a67eef6835f49728cd2147f2688c7cbaa7f639e28d7b < Locally generated hash
          // 997ad7a742beae99a2a67eef6835f49728cd2147f2688c7cb < Recieved hash
          
          if (strncmp((char*)digest, (char*)nodeDigest, 16) == 0) {
            Console.println("HASHES MATCH !!!");
            authSet(sender);

            sprintf(buf, "%s %s %s", deviceID, "CHAP_AUTH", sender);
            rf95.send(buf, sizeof(buf));
            rf95.waitPacketSent();
            Console.println("Notifing the node");

          } else {
            Console.println("Authentication failed");
          }
          
        }
        
      }
      else {
        Console.println("recv failed");
      }
    } else {
      Console.println("Listening...");
    }
  }
}
