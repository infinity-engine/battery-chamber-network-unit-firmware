#ifndef NET_MANAGE_H
#define NET_MANAGE_H
#include "secrets.h"
#include <Arduino.h>
#include "ArduinoJson.h"
#include "functionPrototype.h"

#ifndef ESP
#define ESP
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>
#endif

class NetWorkManager
{
public:
    ESP8266WiFiMulti wifi;
    NetWorkManager();
    bool setup();
    String makePostReq(String url, String jsonString);
    String makeGetReq(String url);
    bool checkInternetConnectivity();
    String fetchExp();
    bool sendMeasurement(u_int8_t channelNo, String measurement);
    bool setStatus(String status, u_int8_t channelNo = 0, u_int8_t rowNo = 0);
    bool resolveResponse(String response);
    bool incrementMultiPlierIndex(u_int8_t channelNo, u_int8_t rowNo = 0);
    String getDriveCycle(u_int8_t channelNo, u_int8_t rowNo);
    String updatedUpto(u_int8_t channelNo);
    WiFiClient client;
    HTTPClient http;
    String base = API_Base;
    String testId;
};

#endif