#define BAUDRATE 115200

#include <Process.h>
#include <Console.h>
#include <SPI.h>
#include <RH_RF95.h>
RH_RF95 rf95;

int MESSAGELENGTH = 60; 
// int MESSAGELENGTH = RH_RF95_MAX_MESSAGE_LEN // <-this must be too big as it breaks the code
int led = A2;
int i;
char deviceID[10] = "GATE001";
char* apiRoot = "https://lora-comm.herokuapp.com/api/";
char* apiKey = "abcdefgHijkLMNOP"; 

void apiCall(int mode, char *parameters, char *result){
  Process p;
  char *apiUrl;
  apiUrl = (char*) malloc(sizeof(char)*200);
  strcpy(apiUrl, apiRoot);
  
  String response;

  if (mode == 1) {
    // Check if authenticate
    strcat(apiUrl, "getAuthStatus.php");
  } else if (mode == 2) {
    // Generate new challenge
    strcat(apiUrl, "getNewChallenge.php");
  } else if (mode == 3) {
    // Generate new challenge
    strcat(apiUrl, "getChallenge.php");
  } else if (mode == 4) {
    // Authenticate a node that sent a CHAL_RESP
    strcat(apiUrl, "authNode.php");
  }

  strcat(apiUrl, "?apiKey=");
  strcat(apiUrl, apiKey);
  strcat(apiUrl, parameters);
  
  //Console.print(" -- ");
  //Console.println(apiUrl);

  p.begin("curl");
  p.addParameter(apiUrl);
  p.run();
  free(apiUrl);

  //Console.print(" -- Resp (");
  //Console.print(p.available());
  //Console.print(") : ");

  for(i=0; i<15; i++) {
    if (p.available()>0) result[i] = (char) p.read();
    else break;
  }
  result[i] = '\0';
  //Console.println(result);
}

bool authenticated(char* nodeID) {
  char *parameters;
  parameters = (char*) malloc(25);
  strcpy(parameters, "&nodeID=");
  strcat(parameters, nodeID);
  char result[15];
  apiCall(1, parameters, result);
  free(parameters);
  if ((String)result == "1") {
    return true;
  } else {
    return false;
  }
}

void getNewChallenge(char *nodeID, char *response) {
  char *parameters;
  parameters = (char*) malloc(25);
  strcpy(parameters, "&nodeID=");
  strcat(parameters, nodeID);
  apiCall(2, parameters, response);
  free(parameters);
}

void getChallenge(char* nodeID, char *response) {
  char *parameters;
  parameters = (char*) malloc(25);
  strcpy(parameters, "&nodeID=");
  strcat(parameters, nodeID);
  apiCall(3, parameters, response);
  free(parameters);
}

bool authenticate(char* nodeID, uint8_t* hash) {
  Process p;
  char result[15];
  char *apiUrl;
  apiUrl = (char*) malloc(sizeof(char)*200);
  strcpy(apiUrl, apiRoot);
  strcat(apiUrl, "authNode.php?apiKey=");
  strcat(apiUrl, apiKey);
  strcat(apiUrl, "&nodeID=");
  strcat(apiUrl, nodeID);
  strcat(apiUrl, "&resp=");
    
  for(i=0; i<32; i++) {
    char tmp[3] = "";
    sprintf(tmp, "%02X", hash[i]);
    strcat(apiUrl, tmp);
  }

  //Console.print(" -- ");
  //Console.println(apiUrl);

  p.begin("curl");
  p.addParameter(apiUrl);
  p.run();
  free(apiUrl);

  Console.print(" -- Resp (");
  Console.print(p.available());
  Console.print(") : ");

  for(i=0; i<15; i++) {
    if (p.available()>0) result[i] = (char) p.read();
    else break;
  }
  result[i] = '\0';
  Console.println(result);

  if ((String)result == "1") {
    return true;
  } else {
    return false;
  }
}

void postData(char* nodeID, char* data) {
  Process p;
  char result[20];
  char *apiUrl;
  apiUrl = (char*) malloc(sizeof(char)*200);
  strcpy(apiUrl, apiRoot);
  strcat(apiUrl, "postData.php?apiKey=");
  strcat(apiUrl, apiKey);
  strcat(apiUrl, "&nodeID=");
  strcat(apiUrl, nodeID);
  strcat(apiUrl, "&data=");

  for(i=0; i<40; i++) {    
    char tmp[5];
    sprintf(tmp, "%c", data[i]);
    if (strcmp(tmp, "/")==0) strcat(apiUrl, "%2F");
    else if (strcmp(tmp, "+")==0) strcat(apiUrl, "%2B");
    else if (strcmp(tmp, "=")==0) strcat(apiUrl, "%3D");
    else strcat(apiUrl, tmp);
  }

  //Console.print(" -- ");
  //Console.println(apiUrl);

  p.begin("curl");
  p.addParameter(apiUrl);
  p.run();
  free(apiUrl);
  
  Console.print(" -- Resp (");
  Console.print(p.available());
  Console.print(") : ");

  for(i=0; i<20; i++) {
    if (p.available()>0) result[i] = (char) p.read();
    else break;
  }
  result[i] = '\0';
  Console.println(result);
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
  rf95.setSpreadingFactor(8);
  rf95.setCodingRate4(5);
}

void loop() {
  uint8_t buf[MESSAGELENGTH];
  uint8_t len = sizeof(buf);
  if (rf95.available()) {
    if (rf95.recv(buf, &len)) {
      digitalWrite(led, HIGH);
      char* sender = strtok(buf, " ");
      char* reqType = strtok(NULL, " ");
      char* content = strtok(NULL, " ");

      Console.print(" - ");
      Console.print(reqType);
      Console.print(" from ");
      Console.println(sender);
      
      // Request type 1: MESSAGE
      //     Data from a node
      if (strcmp(reqType, "MESSAGE") == 0) {
        if (authenticated(sender)) {
          postData(sender, content);
            
        } else {
          Console.println("\tNo Auth");
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
        //getChallenge(sender, challenge);
        getNewChallenge(sender, challenge);
        String message = (String)deviceID + " CHAP_CHAL " + (String)challenge;
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
        if (authenticate(sender, nodeDigest)) {
          String message = (String)deviceID + " CHAP_AUTH " + (String)sender;
          int messageLength = message.length();
          messageLength++;
          uint8_t reply[messageLength];
          message.toCharArray(reply, messageLength);
          rf95.send(reply, messageLength);
          rf95.waitPacketSent();  
        }
      }
      
    }
  }
  digitalWrite(led, LOW);
}
