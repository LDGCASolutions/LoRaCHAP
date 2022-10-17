#define BAUDRATE 115200

#include <SHA256.h>
#include <Process.h>
#include <Console.h>
#include <SPI.h>
#include <RH_RF95.h>
RH_RF95 rf95;

SHA256 sha256;

int MESSAGELENGTH = 60; 
// int MESSAGELENGTH = RH_RF95_MAX_MESSAGE_LEN // <-this must be too big as it breaks the code
int led = A2;
int i;
char deviceID[10] = "GATE001";

void apiCall(int mode, String parameters, char *result){
  Process p;
  String apiUrl = "https://lora-comm.herokuapp.com/api/";
  String apiKey = "abcdefgHijkLMNOP";
  String response;

  if (mode == 1) {
    // Check if authenticate
    apiUrl += "getAuthStatus.php";
  } else if (mode == 2) {
    // Generate new challenge
    apiUrl += "getNewChallenge.php";
  } else if (mode == 3) {
    // Generate new challenge
    apiUrl += "getChallenge.php";
  } else if (mode == 4) {
    // Authenticate a node that sent a CHAL_RESP
    apiUrl += "authNode.php";
  }

  apiUrl += "?apiKey=";
  apiUrl += apiKey;
  apiUrl += parameters;

  p.begin("curl");
  p.addParameter(apiUrl);
  p.run();

  while (p.available()>0) {
    response += (char) p.read();
  }
  response += '\0'; 

  Console.print("Resp (");
  Console.print(response.length());
  Console.print(") : ");
  Console.println(response);  

  for(int i=0; i<response.length(); i++) {
    result[i] = response[i];
  }
//  Console.flush();
}

bool authenticated(char* nodeID) {
  String parameters = "&nodeID=" + (String)nodeID;
  char result[15];
  apiCall(1, parameters, result);
  //Console.print("authenticated(): ");
  //Console.println(result);
  
  if ((String)result == "1") {
    //Console.println("GOOD");
    return true;
  } else {
    //Console.println("bad");
    return false;
  }
}

void getNewChallenge(char *nodeID) {
  String parameters = "&nodeID=" + (String) nodeID;
  char result[15]; 
  apiCall(2, parameters, result);
}

void getChallenge(char* nodeID, char *response) {
  String parameters = "&nodeID=" + (String) nodeID;
  char result[15]; 
  apiCall(3, parameters, result);
  //Console.print("getChallenge(): ");
  //Console.println(result);
  for(int i=0; i<15; i++) {
    response[i] = result[i];
  }
}

bool authenticate(char* nodeID, char* hash) {
  String parameters = "&nodeID=" + (String) nodeID + "&resp=" + (String) hash;
  String result;
//  apiCall(4, parameters, result);
  if (result == "1") return true;
  else return false;
}

void printHash(uint8_t *value) {
  for(i=0; i<32; i++) {
    if (value[i] < 16) Console.print("0");  
    Console.print(value[i], HEX);
  }
  Console.println();
}

void setup() {
  pinMode(led, OUTPUT);
  Bridge.begin(BAUDRATE);
  Console.begin();
  Console.println("Gateway Version 3");
  while (!Console) Console.println("Waiting for Console port");  //Wait for Console port to be available.
  while (!rf95.init()) {
    Console.println("Initialisation of LoRa receiver failed");
    delay(1000);
  }
  rf95.setFrequency(915.0);   
  rf95.setTxPower(13); 
  rf95.setSignalBandwidth(125000);
  rf95.setSpreadingFactor(7);
  rf95.setCodingRate4(5);
}

void loop() {
  uint8_t buf[MESSAGELENGTH];
  uint8_t len = sizeof(buf);
  if (rf95.available()) {
    if (rf95.recv(buf, &len)) {
      digitalWrite(led, HIGH);
      Console.println(".");
      char* sender = strtok(buf, " ");
      char* reqType = strtok(NULL, " ");
      char* content = strtok(NULL, " ");

      // Request type 1: MESSAGE
      //     Data from a node
      if (strcmp(reqType, "MESSAGE") == 0) {
        if (authenticated(sender)) {
          // Post to server
        } else {
          String message = (String)deviceID + " CHAP_FAIL " + (String)sender;
          int messageLength = message.length(); 
          messageLength++;
          uint8_t reply[messageLength];
          message.toCharArray(reply, messageLength);
          rf95.send(reply, messageLength);
          rf95.waitPacketSent();
        }
      }

      // Request type 2: CHAP_REQ
      //     A node initiating CHAP. Respond with a challenge
      if (strcmp(reqType, "CHAP_REQ") == 0) {
        char challenge[15];
        getChallenge(sender, challenge); //getNewChallenge(sender);
        String message = (String)deviceID + " CHAP_CHAL " + (String)challenge; //"E81632AF5A";
        int messageLength = message.length();
        messageLength++;
        uint8_t reply[messageLength];
        message.toCharArray(reply, messageLength);
        rf95.send(reply, messageLength);
        rf95.waitPacketSent();          
      }

      // Request type 3: CHAP_RESP
      //     A node responding with the challenge responce
      if (strcmp(reqType, "CHAP_RESP") == 0) {        
        uint8_t nodeDigest[32];
        memcpy(nodeDigest, &buf[18], 32*sizeof(*nodeDigest));

        Console.print("CHAP_RESP : ");
        for(i=0; i<32; i++) {
          if (nodeDigest[i] < 16) Console.print("0");
          Console.print(nodeDigest[i], HEX);
        }
        Console.println();
        
      }
    }
  }
  digitalWrite(led, LOW);
}
