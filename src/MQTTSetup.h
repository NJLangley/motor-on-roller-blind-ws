#include <ArduinoJson.h>

#include <WiFiClient.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient psclient(espClient); //MQTT client

boolean mqttActive = true;

char mqtt_server[40];       //WIFI config: MQTT server config (optional)
char mqtt_port[6] = "1883"; //WIFI config: MQTT port config (optional)
char mqtt_uid[40];          //WIFI config: MQTT server username (optional)
char mqtt_pwd[40];          //WIFI config: MQTT server password (optional)

void loadMQTT(JsonVariant json)
{
  strcpy(mqtt_server, json["mqtt_server"]);
  strcpy(mqtt_port, json["mqtt_port"]);
  strcpy(mqtt_uid, json["mqtt_uid"]);
  strcpy(mqtt_pwd, json["mqtt_pwd"]);
}

void mqttSetup(void (*processMsg)(String res, uint8_t clientnum))
{
  /* Setup connection for MQTT and for subscribed
    messages IF a server address has been entered
  */
  Serial.println("MQTTTTT " + String(mqtt_server));
  if (String(mqtt_server) != "")
  {
    Serial.println(F("Registering MQTT server"));
    psclient.setServer(mqtt_server, String(mqtt_port).toInt());
    psclient.setCallback([&](char *topic, byte *payload, unsigned int length) {
      Serial.print(F("Message arrived ["));
      Serial.print(topic);
      Serial.print(F("] "));
      String res = "";
      for (int i = 0; i < length; i++)
      {
        res += String((char)payload[i]);
      }
      processMsg(res, 0);
    });
  }
  else
  {
    mqttActive = false;
    Serial.println(F("NOTE: No MQTT server address has been registered. Only using websockets"));
  }
}