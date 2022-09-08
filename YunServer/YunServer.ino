/*
  LoRa Simple Yun Server :
  Support Devices: LG01. 
  
  Example sketch showing how to create a simple messageing server, 
  with the RH_RF95 class. RH_RF95 class does not provide for addressing or
  reliability, so you should only use RH_RF95 if you do not need the higher
  level messaging abilities.

  It is designed to work with the other example LoRa Simple Client

  User need to use the modified RadioHead library from:
  https://github.com/dragino/RadioHead

  modified 16 11 2016
  by Edwin Chen <support@dragino.com>
  Dragino Technology Co., Limited
*/
//If you use Dragino IoT Mesh Firmware, uncomment below lines.
//For product: LG01. 
#define BAUDRATE 115200

//If you use Dragino Yun Mesh Firmware , uncomment below lines. 
//#define BAUDRATE 250000

#include <Console.h>
#include <SPI.h>
#include <RH_RF95.h>
#include <SHA256.h>
SHA256 sha256;

// Singleton instance of the radio driver
RH_RF95 rf95;

int led = A2;
float frequency = 915.0;
bool authenticated = false;

char sender[10];
char reqType[10];
char content[40];
char deviceID[10] = "GATE001";

uint8_t nodeDigest[32];

// The following device password shuould be retrieved from a DB

// NODE001:VerySecurePassword001

char devicePW[] = "VerySecurePassword001";


uint8_t hashdigest(Hash *hash, char *plaintext) {
  // ThisIsARandomString
  // E3ED18D0AE5E96C7BA04C855ABE7C08C34D0FCAC1CE61978836ACF068E1C8E19

  // ThisIsARandomStringVerySecurePassword001
  // 997AD7A742BEAE99A2A67E0EF6835F49728CD2147F2688C7CB0AA7F639E28D7B
  
  uint8_t value[32];

  hash->reset();
  hash->update(plaintext, strlen(plaintext));
  hash->finalize(value, sizeof(value));

  Console.print("Inside hashdigest(): ");
  for(int i; i<sizeof(value); i++) {
    uint8_t letter[1];
    sprintf(letter,"%x",value[i]);  
    Console.print((char*)letter);
  }
  Console.println();

  return value;
}

void setup() 
{
  pinMode(led, OUTPUT);     
  Bridge.begin(BAUDRATE);
  Console.begin();
  // while (!Console) ; // Wait for console port to be available
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
    char challenge[] = "ThisIsARandomString";
    
    if (rf95.recv(buf, sizeof(buf))) {
      digitalWrite(led, HIGH);
      Console.println("=========================================================================");
      Console.print("Message: ");
      Console.println((char*)buf);

      sscanf(buf, "%s %s %s", &sender, &reqType, &content);

      if (strcmp(reqType, "CHAP_REQ") == 0) {
        Console.print(content);
        Console.println(" is trying to authenticate");
        delay(500);
        
        // CHAP Step 1 : Send the challenge
        
        sprintf(buf, "%s %s %s", deviceID, "CHAP_CHAL", challenge);
        rf95.send(buf, sizeof(buf));
        rf95.waitPacketSent();
        Console.println("Challenge sent");
      }

      if (strcmp(reqType, "CHAP_RESP") == 0) {
        Console.print("Challenge response recieved from ");
        Console.println(sender);

//        uint8_t nodeDigest[32];
        sscanf(buf, "%s %s %s", &sender, &reqType, &nodeDigest);

        uint8_t temp1[50];
        sprintf(temp1, "%s%s", challenge, devicePW);
        Console.println((char*)temp1);

        uint8_t digest[32];
        digest[32] = hashdigest(&sha256, temp1);

        Console.print("Recieved hash: ");
        for(int i; i<strlen((char*)nodeDigest); i++) {
          uint8_t letter[1];
          sprintf(letter,"%x",nodeDigest[i]);  
          Console.print((char*)letter);
        }
        Console.println();

        // 997ad7a742beae99a2a67eef6835f49728cd2147f2688c7cbaa7f639e28d7b < Locally generated hash
        // 997ad7a742beae99a2a67eef6835f49728cd2147f2688c7cb < Recieved hash

        Console.print("Recieved hash string length: ");
        Console.println(strlen((char*)nodeDigest));
        
        if (strncmp((char*)digest, (char*)nodeDigest, 10) == 0) {
          Console.println("HASHES MATCH !!!");
        } else {
          Console.println("Hashes dont match :-(");
        }
        
      }

      if (authenticated) {
        // Send a reply
        uint8_t data[] = "Hi NODE001, Let's Talk";
        rf95.send(data, sizeof(data));
        rf95.waitPacketSent();
        Console.println("Sent a reply");
        digitalWrite(led, LOW);
      }
    }
    else
    {
      Console.println("recv failed");
    }
  }
}
