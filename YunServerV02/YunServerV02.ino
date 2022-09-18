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
int i;
char deviceID[10] = "GATE001";

// The following device data shuould be retrieved from a DB

char devices[10][10] = {"NODE001", "NODE002"};
char passwords[10][30] = {"VerySecurePassword001", "PW2"};
char challenges[10][10] = {};
bool authenticated[10] = {};

void genChallenge( char(&space)[10]) {
  for(i=0; i<10; i++) {
    int r = random(65,91); // ASCII Uppercase alphabet range
    space[i] = r;
  }
}

int getDeviceIndex(char *deviceID) {
  for (i=0; i<10; i++) {
    if (memcmp(deviceID, devices[i], strlen(deviceID)) == 0) {
      return i;
    }  
  }
  return -1;
}

void hashdigest(Hash *hash, char *plaintext, uint8_t (&digest) [32]) {
  // ThisIsARandomStringVerySecurePassword001
  // 997AD7A742BEAE99A2A67E0EF6835F49728CD2147F2688C7CB0AA7F639E28D7B
  
  uint8_t value[32];
  
  hash->reset();
  hash->update(plaintext, strlen(plaintext));
  hash->finalize(value, sizeof(value));

//  for(i=0; i<32; i++) {
//    digest[i] = value[i];
//  }

  memcpy(digest, value, 32);

//  Console.print("Inside HashDigest: ");
//  printHash(value);

  return digest;
}

void printHash(uint8_t *value) {
  for(i=0; i<32; i++) {
    if (value[i] < 10) Console.print("0");  
    Console.print(value[i], HEX);
  }
  Console.println();
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

        int deviceIndex = getDeviceIndex(sender);

        if (deviceIndex >= 0) {
          if (strcmp(reqType, "MESSAGE") == 0) {
            // Check the authentication status of the device
            if (authenticated[deviceIndex]) {
              // Device authenticated. Accept the message.
              Console.print(sender);
              Console.println(" is authenticated");
  
              // What happens next depends on what this device is
              
            } else {
              Console.print(sender);
              Console.println(" is NOT authenticated");
              authenticated[deviceIndex] = false;
    
              sprintf(buf, "%s %s %s", deviceID, "CHAP_FAIL", sender);
              rf95.send(buf, sizeof(buf));
              rf95.waitPacketSent();
              Console.println("CHAP reset request sent");
            }
          }
  
          if (strcmp(reqType, "CHAP_REQ") == 0) {
            Console.print(content);
            Console.println(" is trying to authenticate");
                        
            genChallenge(challenges[deviceIndex]);
            
            Console.print("Challenge: ");
            Console.println((char*)challenges[deviceIndex]);
            
            sprintf(buf, "%s %s %s", deviceID, "CHAP_CHAL", challenges[deviceIndex]);
            rf95.send(buf, sizeof(buf));
            rf95.waitPacketSent();
            Console.println("Challenge sent");
          }
  
          if (strcmp(reqType, "CHAP_RESP") == 0) {
            Console.print("Challenge response recieved from ");
            Console.println(sender);
            Console.println((char*)challenges[deviceIndex]);
            // Console.println((char*)passwords[deviceIndex]); // <=== Problem :-(
            

            uint8_t nodeDigest[32];
            memcpy(nodeDigest, &buf[18], 32*sizeof(*nodeDigest));
  
            Console.print("Recieved hash: ");
            printHash(nodeDigest);

//            uint8_t temp[40];
//            sprintf(temp, "%s%s", challenges[deviceIndex], passwords[deviceIndex]);
//            Console.println((char*)temp);
    
//            uint8_t digest[32];
//            hashdigest(&sha256, temp, digest);
    
            // 997ad7a742beae99a2a67eef6835f49728cd2147f2688c7cbaa7f639e28d7b < Locally generated hash
            // 997ad7a742beae99a2a67eef6835f49728cd2147f2688c7cb < Recieved hash
            
//            if (memcmp(digest, nodeDigest, 32) == 0) {
//              Console.println("HASHES MATCH !!!");
//              authenticated[deviceIndex] = true;
//  
////              sprintf(buf, "%s %s %s", deviceID, "CHAP_AUTH", sender);
////              rf95.send(buf, sizeof(buf));
////              rf95.waitPacketSent();
////              Console.println("Notifing the node");
//  
//            } else {
//              authenticated[deviceIndex] = false;
//              Console.println("Authentication failed");
//            }
          }
        } else {
          Console.println("\tUNKNOWN DEVICE\n");
        }
      } else {
        Console.println("Recv failed...");
      }
    } else {
      Console.println("Timeout...");
    }
  }
}
