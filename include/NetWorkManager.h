#ifndef NET_MANAGE_H
#define NET_MANAGE_H
#include "secrets.h"
#include <Arduino.h>
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
    String makePostReq(String url);
    String makeGetReq(String url);
    void test();
    void fetchExp();
    WiFiClient client;
    HTTPClient http;
    String base = API_Base;
};

#endif