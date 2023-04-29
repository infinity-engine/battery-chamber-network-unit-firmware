#ifndef NET_MANAGE_H
#define NET_MANAGE_H

#include "secrets.h"
#include <Arduino.h>
#include "ArduinoJson.h"
#include "functionPrototype.h"

#ifndef ESP
#define ESP
#include <ESP8266WiFi.h>
#include <AsyncHTTPRequest_Generic.hpp>
#endif

#ifndef Req_CB
#define Req_CB
void requestCB0(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);
void requestCB1(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);
void requestCB2(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);
void requestCB3(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);
void requestCB4(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);
void requestCB5(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);
void requestCB6(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);

typedef void (*requestCallback)(void *optParm, AsyncHTTPRequest *thisRequest, int readyState);
typedef void (*sendCallback)();

#endif
extern requestCallback requestCB[MAX_NO_CHANNELS + 1];
extern bool readyToSend[MAX_NO_CHANNELS + 1];
extern bool isPrevReqSuccess[MAX_NO_CHANNELS + 1];
extern AsyncHTTPRequest request[MAX_NO_CHANNELS + 1];

#define ASYNC_HTTP_DEBUG_PORT Serial

#define _ASYNC_HTTP_LOGLEVEL_ 0

class NetWorkManager
{
public:
    NetWorkManager();
    bool setup();
    void sendRequest(const char *method, const char *url, const char *body = "", uint8_t reqChannel = 0, bool isSynchronous = true);
    char *makePostReq(String url, const char *body = "");
    char *makeGetReq(String url);
    bool checkAPIConnectivity();
    char *fetchExp();
    void sendMeasurement(u_int8_t channelNo, const char *);
    void setStatus(String status, u_int8_t channelNo = 0, u_int8_t rowNo = 0);
    bool resolveResponse(String response);
    void incrementMultiPlierIndex(u_int8_t channelNo, u_int8_t rowNo = 0);
    char *getDriveCycle(u_int8_t channelNo, u_int8_t rowNo);
    char *updatedUpto(u_int8_t channelNo);
    String base = API_Base;
    String testId;
};

#endif