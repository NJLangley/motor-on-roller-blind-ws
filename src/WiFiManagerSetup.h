
#include <WiFi.h>
#include <WiFiManager.h>
#include <WiFiUdp.h>

//Configure Default Settings for Access Point logon
String APid = "Blinds AP"; //Name of access point
String APpw = "";          //Hardcoded password for access point

char config_name[40] = "blinds";    //WIFI config: Bonjour name of device
char config_rotation[40] = "false"; //WIFI config: Detault rotation is CCW

WiFiManagerParameter custom_config_name();

String *wifiManagerSetup(void (*saveConfigCallback)(), char *mqtt_server, char *mqtt_port)
{

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

  String paramsValues[8] = {
      custom_config_name.getValue(),
      custom_rotation.getValue(),
      custom_text.getValue(),
      custom_mqtt_server.getValue(),
      custom_mqtt_port.getValue(),
      custom_mqtt_uid.getValue(),
      custom_mqtt_pwd.getValue(),
      custom_text2.getValue()};

  return paramsValues;
}