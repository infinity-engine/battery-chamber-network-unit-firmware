#if !(defined(ESP8266) || defined(ESP32))
#error This code is intended to run on the ESP8266 or ESP32 platform! Please check your Tools->Board setting.
#endif

// Level from 0-4
#define ASYNC_HTTP_DEBUG_PORT Serial
#define _ASYNC_HTTP_LOGLEVEL_ 4

// 300s = 5 minutes to not flooding
#define HTTP_REQUEST_INTERVAL 60 // 300

// 10s
#define HEARTBEAT_INTERVAL 10

int status; // the Wifi radio's status
#include "secrets.h"
const char *ssid = WiFi_SSID;
const char *password = WiFi_PWD;

#if (ESP8266)
#include <ESP8266WiFi.h>
#elif (ESP32)
#include <WiFi.h>
#endif

#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN_TARGET "AsyncHTTPRequest_Generic v1.10.2"
#define ASYNC_HTTP_REQUEST_GENERIC_VERSION_MIN 1010002

// Seconds for timeout, default is 3s
#define DEFAULT_RX_TIMEOUT 10

// Uncomment for certain HTTP site to optimize
// #define NOT_SEND_HEADER_AFTER_CONNECTED        true

// To be included only in main(), .ino with setup() to avoid `Multiple Definitions` Linker Error
#include <AsyncHTTPRequest_Generic.h> // https://github.com/khoih-prog/AsyncHTTPRequest_Generic

#include <Ticker.h>

AsyncHTTPRequest request;
Ticker ticker;
Ticker ticker1;

void heartBeatPrint()
{
  static int num = 1;

  if (WiFi.status() == WL_CONNECTED)
    Serial.print(F("H")); // H means connected to WiFi
  else
    Serial.print(F("F")); // F means not connected to WiFi

  if (num == 80)
  {
    Serial.println();
    num = 1;
  }
  else if (num++ % 10 == 0)
  {
    Serial.print(F(" "));
  }
}

void sendRequest(const char *url)
{
  static bool requestOpenResult;

  if (request.readyState() == readyStateUnsent || request.readyState() == readyStateDone)
  {
    // requestOpenResult = request.open("GET", "http://worldtimeapi.org/api/timezone/Europe/London.txt");
    requestOpenResult = request.open("GET", url);

    if (requestOpenResult)
    {
      // Only send() if open() returns true, or crash
      request.send();
    }
    else
    {
      Serial.println(F("Can't send bad request"));
    }
  }
  else
  {
    Serial.println(F("Can't send request"));
  }
}

void requestCB(void *optParm, AsyncHTTPRequest *request, int readyState)
{
  (void)optParm;

  if (readyState == readyStateDone)
  {
    AHTTP_LOGDEBUG(F("\n**************************************"));
    AHTTP_LOGDEBUG1(F("Response Code = "), request->responseHTTPString());

    if (request->responseHTTPcode() == 200)
    {
      Serial.println(F("\n**************************************"));
      Serial.println(request->responseText());
      Serial.println(F("**************************************"));
    }
  }
}

void setup()  
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  while (!Serial && millis() < 5000)
    ;

  Serial.print(F("\nStarting AsyncHTTPRequest_ESP using "));
  Serial.println(ARDUINO_BOARD);
  Serial.println(ASYNC_HTTP_REQUEST_GENERIC_VERSION);

  WiFi.mode(WIFI_STA);

  WiFi.begin(ssid, password);

  Serial.print(F("Connecting to WiFi SSID: "));
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(F("."));
  }

  Serial.print(F("\nAsyncHTTPRequest @ IP : "));
  Serial.println(WiFi.localIP());

  request.setDebug(false);

  request.onReadyStateChange(requestCB);

  // Send first request now
}

void loop()
{
  sendRequest("http://worldtimeapi.org/api/timezone/America/Toronto.txt");
  delay(1000);
  sendRequest("http://worldtimeapi.org/api/timezone/Europe/London.txt");
  delay(1000);
}