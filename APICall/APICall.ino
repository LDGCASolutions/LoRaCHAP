#include <SPI.h>
#include <Console.h>
#include <Process.h>
Process p;

#define BAUDRATE 115200

int apiCall(int mode, String parameters)
{
  String apiUrl = "https://lora-comm.herokuapp.com/api/";
  String apiKey = "abcdefgHijkLMNOP";
  char response;

  if (mode == 1) {
    // Check if authenticate
    apiUrl += "getAuthStatus.php?apiKey=" + apiKey + parameters;
  }

  p.begin("curl");
  p.addParameter(apiUrl);
  p.run();
  
  while (p.available()>0) {
    response = p.read();
  }
  
  if (response == "1") return 1;
  else return 0;
}

void setup()
{
    Bridge.begin(BAUDRATE);
    Console.begin(); 
    while(!Console);

    int response = apiCall(1,"&nodeID=NOED001");
    if (response > 0) Console.println("NODE001 Authenticated");
    else Console.println("NODE001 Not Auth");
      
}

void loop()
{


}
