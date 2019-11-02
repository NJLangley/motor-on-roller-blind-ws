#include <WebSocketsServer.h>

WebSocketsServer webSocket = WebSocketsServer(81); // WebSockets will respond on port 81

void webSocketsSetup(void (*processMsg)(String res, uint8_t clientnum))
{

  //Start websocket
  webSocket.begin();
  webSocket.onEvent([&](uint8_t num, WStype_t type, uint8_t *payload, size_t length) {
    switch (type)
    {
    case WStype_TEXT:
      Serial.printf("[%u] get Text: %s\n", num, payload);

      String res = (char *)payload;

      //Send to common MQTT and websocket function
      processMsg(res, num);
      break;
    }
  });
}