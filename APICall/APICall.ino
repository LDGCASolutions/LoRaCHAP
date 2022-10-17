#include <SPI.h>
#include <Console.h>
#include <Process.h>
Process p;

#define BAUDRATE 115200

String apiCall(int mode, String parameters){
  String apiUrl = "https://lora-comm.herokuapp.com/api/";
  String apiKey = "abcdefgHijkLMNOP";
  String response;

  if (mode == 1) {
    // Check if authenticate
    apiUrl += "getAuthStatus.php?apiKey=" + apiKey + parameters;
  } else if (mode == 2) {
    // Generate new challenge
    apiUrl += "getNewChallenge.php?apiKey=" + apiKey + parameters;
  } else if (mode == 3) {
    // Generate new challenge
    apiUrl += "getChallenge.php?apiKey=" + apiKey + parameters;
  } else if (mode == 4) {
    // Authenticate a node that sent a CHAL_RESP
    apiUrl += "authNode.php?apiKey=" + apiKey + parameters;
  }

  p.begin("curl");
  p.addParameter(apiUrl);
  p.run();
  
  while (p.available()>0) {
    response += (char) p.read();
  }
  
  return response;
}

bool authenticated(char* nodeID) {
  String parameters = "&nodeID=" + (String) nodeID;
  String result = apiCall(1, parameters);
  Console.print("Response: ");
  Console.println(result);
  if (result == "1") return true;
  else return false;
}

String getNewChallenge(char *nodeID) {
  String parameters = "&nodeID=" + (String) nodeID;
  String result = apiCall(2, parameters);
  Console.print("New Challenge: ");
  Console.println(result);
  return result;
}

String getChallenge(char* nodeID) {
  String parameters = "&nodeID=" + (String) nodeID;
  String result = apiCall(3, parameters);
  return result;
}

bool authenticate(char* nodeID, char* hash) {
  String parameters = "&nodeID=" + (String) nodeID + "&resp=" + (String) hash;
  String result = apiCall(4, parameters);
  if (result == "1") return true;
  else return false;
}

void setup()
{
    Bridge.begin(BAUDRATE);
    Console.begin(); 
    while(!Console);

    char* nodeID = "NODE002";  
    authenticated(nodeID);
}

void loop()
{
  Console.println("==============================");
  char* nodeID = "NODE002";  
  authenticated(nodeID);
  delay(1000);
}
