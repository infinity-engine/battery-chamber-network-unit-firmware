#ifndef NET_MANAGE_H
#define NET_MANAGE_H
#include "secrets.h"
#include <Arduino.h>
#include "ArduinoJson.h"

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
    String makePostReq(String url, String JsonString);
    String makeGetReq(String url);
    bool checkInternetConnectivity();
    void fetchExp();
    bool sendMeasurement(u_int8_t channelNo, String measurement);
    WiFiClient client;
    HTTPClient http;
    String base = API_Base;
    String testId;
};

#endif