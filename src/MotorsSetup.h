#include <ArduinoJson.h>

#include "WebSocketsServer.h"
#include <Stepper_28BYJ_48.h>

int M1_1 = 13;
int M1_2 = 12;
int M1_3 = 14;
int M1_4 = 27;

Stepper_28BYJ_48 stepper_1(M1_1, M1_2, M1_3, M1_4); //Initiate stepper driver

String action; //Action manual/auto
int path = 0;  //Direction of blind (1 = down, 0 = stop, -1 = up)

int setPos = 0;             //The set position 0-100% by the client
long currentPosition = 0;   //Current position of the blind
long maxPosition = 2000000; //Max position of the blind. Initial value
boolean ccw = true;         //Turns counter clockwise to lower the curtain

void loadMotors(JsonVariant json)
{
  currentPosition = json["currentPosition"].as<long>();
  maxPosition = json["maxPosition"].as<long>();
}

/* Turn of power to coils whenever the blind is not moving */
void stopPowerToCoils()
{
  digitalWrite(M1_1, LOW);
  digitalWrite(M1_2, LOW);
  digitalWrite(M1_3, LOW);
  digitalWrite(M1_4, LOW);
  Serial.println(F("Motor stopped"));
}

void motors_broadcast(WebSocketsServer webSocket, String outputTopic, void (*sendmsg)(String topic, String payload))
{
  int set = (setPos * 100) / maxPosition;
  int pos = (currentPosition * 100) / maxPosition;
  webSocket.broadcastTXT("{ \"set\":" + String(set) + ", \"position\":" + String(pos) + " }");
  sendmsg(outputTopic, "{ \"set\":" + String(set) + ", \"position\":" + String(pos) + " }");
  Serial.println(F("Stopped. Reached wanted position"));
}
void motors_up()
{
  Serial.println(F("Moving up"));
  stepper_1.step(ccw ? 1 : -1);
  currentPosition = currentPosition + 1;
}
void motors_down()
{
  Serial.println(F("Moving down"));
  stepper_1.step(ccw ? -1 : 1);
  currentPosition = currentPosition - 1;
}

void motors_auto(int path)
{
  if (currentPosition > path)
  {
    motors_down();
  }
  else if (currentPosition < path)
  {
    motors_up();
  }
}
void motors_manual()
{
  Serial.println(F("Moving motor manually"));
  stepper_1.step(ccw ? path : -path);
  currentPosition = currentPosition + path;
}

boolean motorsLoop(uint8_t btndn, uint8_t btnup, WebSocketsServer webSocket, String outputTopic, void (*sendmsg)(String topic, String payload))
{

  bool pres_cont = false;
  while (!digitalRead(btndn) && currentPosition > 0)
  {
    motors_down();
    yield();
    delay(1);
    pres_cont = true;
  }
  while (!digitalRead(btnup) && currentPosition < maxPosition)
  {
    motors_up();
    yield();
    delay(1);
    pres_cont = true;
  }
  if (pres_cont)
  {
    motors_broadcast(webSocket, outputTopic, sendmsg);
  }

  return pres_cont;
}