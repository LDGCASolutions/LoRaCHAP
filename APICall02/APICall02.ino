#include <SPI.h>
#include <Console.h>
#include <Process.h>
Process p;

#define BAUDRATE 115200

void apiCall(int mode, String parameters, char *result){
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

  Console.print("Resp: ");
  Console.println(response);
  Console.println(response.length());

  for(int i=0; i<response.length(); i++) {
    result[i] = response[i];
  }
}

bool authenticated(char* nodeID) {
  String parameters = "&nodeID=" + (String)nodeID;
  char result[15];
  apiCall(1, parameters, result);
  Console.print("authenticated(): ");
  Console.println(result);
  
  if ((String)result == "1") {
    Console.println("GOOD");
    return true;
  } else {
    Console.println("bad");
    return false;
  }
}

void getNewChallenge(char *nodeID) {
  //String parameters = "&nodeID=" + (String) nodeID;
  //char* result = apiCall(2, parameters);
  //return result;
}

void getChallenge(char* nodeID, char *response) {
  String parameters = "&nodeID=" + (String) nodeID;
  char result[15]; 
  apiCall(3, parameters, result);
  Console.print("getChallenge(): ");
  Console.println(result);
  for(int i=0; i<15; i++) {
    response[i] = result[i];
  }
}

bool authenticate(char* nodeID, char* hash) {
  String parameters = "&nodeID=" + (String) nodeID + "&resp=" + (String) hash;
  String result;
//  apiCall(4, parameters, result);
  Console.print("authenticate():");
  Console.println(result);
  if (result == "1") return true;
  else return false;
}

void setup()
{
    Bridge.begin(BAUDRATE);
    Console.begin(); 
    while(!Console);

    
}

void loop()
{
  Console.println("==============================");
  char* nodeID = "NODE001";
  if (true) {
    if (true) {
      if(true) {
        Console.println(".");
        if (authenticated(nodeID)) {
          Console.println("Authenticated");
        } else {
          Console.println("NOT Authenticated");
        }        
      }

      if (true) {
        char response[15];
        getChallenge(nodeID, response);
        Console.println(response);
      }
    }
  }
  delay(1000);
}
