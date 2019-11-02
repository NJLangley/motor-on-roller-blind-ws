#include <WiFiClient.h>
#include <PubSubClient.h>
#include <WebServer.h>
#include "index_html.h"
#include <ESPmDNS.h>
#include <WiFi.h>

// Version number for checking if there are new code releases and notifying the user
String version = "1.3.3";

//Fixed settings for WIFI
WiFiClient espClient;
PubSubClient psclient(espClient); //MQTT client

WebServer server(80); // TCP server at port 80 will respond to HTTP requests

void handleRoot()
{
  server.send(200, "text/html", INDEX_HTML);
}
void handleNotFound()
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++)
  {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void serverSetup(char *config_name)
{
  /*
    Setup multi DNS (Bonjour)
  */
  if (MDNS.begin(config_name))
  {
    Serial.println(F("MDNS responder started"));
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws", "tcp", 81);
  }
  else
  {
    Serial.println(F("Error setting up MDNS responder!"));
    while (1)
    {
      delay(1000);
    }
  }
  Serial.print("Connect to http://" + String(config_name) + ".local or http://");
  Serial.println(WiFi.localIP());

  //Start HTTP server
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);

  server.begin();

  //Update webpage
  INDEX_HTML.replace("{VERSION}", "V" + version);
  INDEX_HTML.replace("{NAME}", String(config_name));
}