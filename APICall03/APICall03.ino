#include <SPI.h>
#include <Console.h>
#include <Process.h>
Process p;
int i;

#define BAUDRATE 115200

void apiCall(int mode, char* parameters, char* result){
  char apiUrl[200] = "https://lora-comm.herokuapp.com/api/";
  char apiKey[] = "abcdefgHijkLMNOP";
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
  
  Console.print(" -- ");
  Console.println(apiUrl);  

  p.begin("curl");
  p.addParameter(apiUrl);
  p.run();

  Console.print("Resp (");
  Console.print(p.available());
  Console.print(") : ");

  for(i=0; i<15; i++) {
    if (p.available()>0) result[i] = (char) p.read();
    else break;
  }
  Console.println(result);
}

bool authenticated(char* nodeID) {
  char parameters[20] = "&nodeID=";
  strcat(parameters, nodeID);
  
  char result[15];
  apiCall(1, parameters, result);
  
  if ((String)result == "1") {
    //Console.println("GOOD");
    return true;
  } else {
    //Console.println("bad");
    return false;
  }
}

void getNewChallenge(char *nodeID) {
  char parameters[20] = "&nodeID=";
  strcat(parameters, nodeID);
  
  char result[15]; 
  //apiCall(2, parameters, result);
}

void getChallenge(char* nodeID, char* response) {
  char parameters[20] = "&nodeID=";
  strcat(parameters, nodeID);
  
  char result[15]; 
  apiCall(3, parameters, result);
  //Console.print("getChallenge(): ");
  //Console.println(result);
  for(int i=0; i<15; i++) {
    response[i] = result[i];
  }
}

bool authenticate(char* nodeID, uint8_t* hash) {  
  char parameters[70] = "&nodeID=";
  strcat(parameters, nodeID);
  strcat(parameters, "&resp=");
  strcat(parameters, "BE219746E091F672E30CED84C89B3C96B0B08D74062A7C96FEA7013CDE89B16G");

  Console.print(" -- ");
  Console.println(parameters);
  
  char result[15]; 
  apiCall(4, parameters, result);

  if ((String)result == "1") {
    //Console.println("GOOD");
    return true;
  } else {
    //Console.println("bad");
    return false;
  }
}

void setup() {
    Bridge.begin(BAUDRATE);
    Console.begin(); 
    while(!Console);    
}

void loop() {
  Console.println("==============================");
  char* nodeID = "NODE001";
  char* hashString = "";
  char response[15];
  
  authenticated(nodeID);
  
  getChallenge(nodeID, response);
  
  delay(1000);
}
