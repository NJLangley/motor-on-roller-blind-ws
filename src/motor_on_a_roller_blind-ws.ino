#include <ArduinoJson.h>
#include <WiFi.h>
#include <WebSocketsServer.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>
#include "FS.h"
#include "SPIFFS.h"
#include "NidayandHelper.h"

#include "MotorsSetup.h"
#include "WebServerSetup.h"
#include "OTASetup.h"
#include "MQTTSetup.h"

//Configure Default Settings for Access Point logon
String APid = "Blinds AP"; //Name of access point
String APpw = "";          //Hardcoded password for access point

char config_name[40] = "blinds";    //WIFI config: Bonjour name of device
char config_rotation[40] = "false"; //WIFI config: Detault rotation is CCW

//Set up buttons
const uint8_t btnup = 18;  //Up button
const uint8_t btndn = 19;  //Down button
const uint8_t btnres = 20; //Reset button

//----------------------------------------------------

NidayandHelper helper = NidayandHelper();

String outputTopic; //MQTT topic for sending messages
String inputTopic;  //MQTT topic for listening

boolean loadDataSuccess = false;
boolean saveItNow = false;     //If true will store positions to SPIFFS
bool shouldSaveConfig = false; //Used for WIFI Manager callback to save parameters
boolean initLoop = true;       //To enable actions first time the loop is run

WebSocketsServer webSocket = WebSocketsServer(81); // WebSockets will respond on port 81

bool loadConfig()
{
  if (!helper.loadconfig())
    return false;

  JsonVariant json = helper.getconfig();

  //Useful if you need to see why confing is read incorrectly
  json.printTo(Serial);

  //Store variables locally
  strcpy(config_name, json["config_name"]);
  strcpy(config_rotation, json["config_rotation"]);
  loadMotors(json);
  loadMQTT(json);

  return true;
}

/**
   Save configuration data to a JSON file
   on SPIFFS
*/
bool saveConfig()
{
  DynamicJsonBuffer jsonBuffer(300);
  JsonObject &json = jsonBuffer.createObject();
  json["config_name"] = config_name;
  json["config_rotation"] = config_rotation;

  json["currentPosition"] = currentPosition;
  json["maxPosition"] = maxPosition;

  json["mqtt_server"] = mqtt_server;
  json["mqtt_port"] = mqtt_port;
  json["mqtt_uid"] = mqtt_uid;
  json["mqtt_pwd"] = mqtt_pwd;

  return helper.saveconfig(json);
}

/*
   Connect to MQTT server and publish a message on the bus.
   Finally, close down the connection and radio
*/
void sendmsg(String topic, String payload)
{
  if (!mqttActive)
    return;

  helper.mqtt_publish(psclient, topic, payload);
}

/****************************************************************************************
*/
void processMsg(String res, uint8_t clientnum)
{
  /*
     Check if calibration is running and if stop is received. Store the location
  */
  if (action == "set" && res == "(0)")
  {
    maxPosition = currentPosition;
    saveItNow = true;
  }

  //Below are actions based on inbound MQTT payload
  if (res == "(start)")
  {

    //Store the current position as the start position
    currentPosition = 0;
    path = 0;
    saveItNow = true;
    action = "manual";
  }
  else if (res == "(max)")
  {

    //Store the max position of a closed blind
    maxPosition = currentPosition;
    path = 0;
    saveItNow = true;
    action = "manual";
  }
  else if (res == "(0)")
  {

    //Stop
    path = 0;
    saveItNow = true;
    action = "manual";
  }
  else if (res == "(1)")
  {

    //Move down without limit to max position
    path = 1;
    action = "manual";
  }
  else if (res == "(-1)")
  {

    //Move up without limit to top position
    path = -1;
    action = "manual";
  }
  else if (res == "(update)")
  {
    //Send position details to client
    int set = (setPos * 100) / maxPosition;
    int pos = (currentPosition * 100) / maxPosition;
    sendmsg(outputTopic, "{ \"set\":" + String(set) + ", \"position\":" + String(pos) + " }");
    webSocket.sendTXT(clientnum, "{ \"set\":" + String(set) + ", \"position\":" + String(pos) + " }");
  }
  else if (res == "(ping)")
  {
    //Do nothing
  }
  else
  {
    /*
       Any other message will take the blind to a position
       Incoming value = 0-100
       path is now the position
    */
    path = maxPosition * res.toInt() / 100;
    setPos = path; //Copy path for responding to updates
    action = "auto";

    motors_broadcast(webSocket, outputTopic, sendmsg);
  }
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{
  switch (type)
  {
  case WStype_TEXT:
    Serial.printf("[%u] get Text: %s\n", num, payload);

    String res = (char *)payload;

    //Send to common MQTT and websocket function
    processMsg(res, num);
    break;
  }
}

/*
   Callback from WIFI Manager for saving configuration
*/
void saveConfigCallback()
{
  shouldSaveConfig = true;
}

void setup(void)
{
  Serial.begin(115200);
  delay(100);
  Serial.print(F("Starting now\n"));

  pinMode(btnup, INPUT_PULLUP);
  pinMode(btndn, INPUT_PULLUP);
  pinMode(btnres, INPUT_PULLUP);

  //Reset the action
  action = "";

  //Set MQTT properties
  outputTopic = helper.mqtt_gettopic("out");
  inputTopic = helper.mqtt_gettopic("in");

  //Set the WIFI hostname
  WiFi.setHostname(config_name);

  //Define customer parameters for WIFI Manager
  WiFiManagerParameter custom_config_name("Name", "Bonjour name", config_name, 40);
  WiFiManagerParameter custom_rotation("Rotation", "Clockwise rotation", config_rotation, 40);
  WiFiManagerParameter custom_text("<p><b>Optional MQTT server parameters:</b></p>");
  WiFiManagerParameter custom_mqtt_server("server", "MQTT server", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_port("port", "MQTT port", mqtt_port, 6);
  WiFiManagerParameter custom_mqtt_uid("uid", "MQTT username", mqtt_server, 40);
  WiFiManagerParameter custom_mqtt_pwd("pwd", "MQTT password", mqtt_server, 40);
  WiFiManagerParameter custom_text2("<script>t = document.createElement('div');t2 = document.createElement('input');t2.setAttribute('type', 'checkbox');t2.setAttribute('id', 'tmpcheck');t2.setAttribute('style', 'width:10%');t2.setAttribute('onclick', \"if(document.getElementById('Rotation').value == 'false'){document.getElementById('Rotation').value = 'true'} else {document.getElementById('Rotation').value = 'false'}\");t3 = document.createElement('label');tn = document.createTextNode('Clockwise rotation');t3.appendChild(t2);t3.appendChild(tn);t.appendChild(t3);document.getElementById('Rotation').style.display='none';document.getElementById(\"Rotation\").parentNode.insertBefore(t, document.getElementById(\"Rotation\"));</script>");
  //Setup WIFI Manager
  WiFiManager wifiManager;

  //reset settings - for testing
  //clean FS, for testing
  //helper.resetsettings(wifiManager);

  wifiManager.setSaveConfigCallback(saveConfigCallback);
  //add all your parameters here
  wifiManager.addParameter(&custom_config_name);
  wifiManager.addParameter(&custom_rotation);
  wifiManager.addParameter(&custom_text);
  wifiManager.addParameter(&custom_mqtt_server);
  wifiManager.addParameter(&custom_mqtt_port);
  wifiManager.addParameter(&custom_mqtt_uid);
  wifiManager.addParameter(&custom_mqtt_pwd);
  wifiManager.addParameter(&custom_text2);

  wifiManager.autoConnect(APid.c_str(), APpw.c_str());

  Serial.println("--- setup wifiManager done");

  //Load config upon start
  if (!SPIFFS.begin(true))
  {
    Serial.println(F("Failed to mount file system"));
    return;
  }
  Serial.println("--- SPIFFS mounted");

  /* Save the config back from WIFI Manager.
      This is only called after configuration
      when in AP mode
  */
  if (shouldSaveConfig)
  {
    //read updated parameters
    strcpy(config_name, custom_config_name.getValue());
    strcpy(mqtt_server, custom_mqtt_server.getValue());
    strcpy(mqtt_port, custom_mqtt_port.getValue());
    strcpy(mqtt_uid, custom_mqtt_uid.getValue());
    strcpy(mqtt_pwd, custom_mqtt_pwd.getValue());
    strcpy(config_rotation, custom_rotation.getValue());

    //Save the data
    saveConfig();
  }

  Serial.println("--- wifiManager saved config" + String(shouldSaveConfig));

  /*
     Try to load FS data configuration every time when
     booting up. If loading does not work, set the default
     positions
  */
  loadDataSuccess = loadConfig();
  if (!loadDataSuccess)
  {
    Serial.println(F("Unable to load saved data"));
    currentPosition = 0;
    maxPosition = 2000000;
  }

  serverSetup(config_name);

  //Start websocket
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  mqttSetup(processMsg);

  //Set rotation direction of the blinds
  if (String(config_rotation) == "false")
    ccw = true;
  else
    ccw = false;

  otaSetup(config_name);
}

void loop(void)
{
  //OTA client code
  ArduinoOTA.handle();

  //Websocket listner
  webSocket.loop();

  //Serving the webpage
  server.handleClient();

  //MQTT client
  if (mqttActive)
    helper.mqtt_reconnect(psclient, mqtt_uid, mqtt_pwd, {inputTopic.c_str()});

  if (digitalRead(btnres))
  {
    saveItNow = motorsLoop(btndn, btnup, webSocket, outputTopic, sendmsg);
  }

  if (!(digitalRead(btnres) || digitalRead(btndn) || digitalRead(btnup)))
  {
    Serial.println(F("Hold to reset..."));
    uint32_t restime = millis();
    while (!(digitalRead(btnres) || digitalRead(btndn) || digitalRead(btnup)))
      yield(); //Prevent watchdog trigger

    if (millis() - restime >= 2500)
    {
      stopPowerToCoils();
      Serial.println(F("Removing configs..."));

      WiFi.disconnect(true);
      WiFiManager wifiManager;
      helper.resetsettings(wifiManager);

      Serial.println(F("Reboot"));
      // TODO ESP.wdtFeed();
      yield();
      ESP.restart();
    }
  }

  //Storing positioning data and turns off the power to the coils
  if (saveItNow)
  {
    saveConfig();
    saveItNow = false;

    stopPowerToCoils();
  }

  //Manage actions. Steering of the blind
  if (action == "auto")
  {

    if (currentPosition == path)
    {
      path = 0;
      action = "";
      motors_down();
      saveItNow = true;
    }
    else
    {
      motors_auto(path);
    }
  }
  else if (action == "manual" && path != 0)
  {
    motors_manual();
    motors_broadcast(webSocket, outputTopic, sendmsg);
  }

  if (initLoop)
  {
    initLoop = false;
    stopPowerToCoils();
  }
}
