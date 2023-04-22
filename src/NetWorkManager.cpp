#include "NetWorkManager.h"
#include <AsyncHTTPRequest_Generic.h>
// connection different host than the firs one always raise an error.
// default is 3s
#define DEFAULT_RX_TIMEOUT 10

requestCallback requestCB[MAX_NO_CHANNELS + 1] = {requestCB0, requestCB1, requestCB2, requestCB3, requestCB4, requestCB5, requestCB6};
bool readyToSend[MAX_NO_CHANNELS + 1];
bool isPrevReqSuccess[MAX_NO_CHANNELS + 1];
AsyncHTTPRequest request[MAX_NO_CHANNELS + 1];
char emptyStr[] = "";
char *responseText_0;
NetWorkManager::NetWorkManager() {}

bool NetWorkManager::setup()
{
    responseText_0 = emptyStr;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WiFi_SSID, WiFi_PWD);
    while (WiFi.status() != WL_CONNECTED)
    {
        IS_LOG_ENABLED ? Serial.print('.') : 0;
        delay(1000);
    }
    IS_LOG_ENABLED ? Serial.println(F("\nWiFi connection success!")) : 0;
    for (uint8_t i = 0; i <= MAX_NO_CHANNELS; i++)
    {
        AsyncHTTPRequest myReq;
        request[i] = myReq;
        readyToSend[i] = true;
        isPrevReqSuccess[i] = true;
        request[i].setDebug(IS_LOG_ENABLED);
        request[i].onReadyStateChange(requestCB[i]);
    }
    while (!checkAPIConnectivity())
    {
        IS_LOG_ENABLED ? Serial.println(F("Could not connect to API.")) : 0;
        delay(1000);
    }
    IS_LOG_ENABLED ? Serial.println(F("Connection to API is successful.")) : 0;
    testId = "";
    return true;
}

// on channel 0
char *NetWorkManager::makePostReq(String url, const char *body)
{
    // on average taking around 500ms-1s max
    if (WiFi.status() == WL_CONNECTED)
    {
        sendRequest("POST", url.c_str(), body);
    }
    return responseText_0;
}

// on channel 0
char *NetWorkManager::makeGetReq(String url)
{
    if (WiFi.status() == WL_CONNECTED)
    {
        sendRequest("GET", url.c_str());
    }
    return responseText_0;
}

/**
 * @brief Fetch Experiment from cloud, which has status "Scheduled"
 * the cloud will sort all the experiment accroding to scheduled date and return
 * the experiment which has earliest date
 * @return EXP_CONFIG|"null"
 */
char *NetWorkManager::fetchExp()
{
    const char *base = API_Base;
    String url = String(base) + "is-any-experiment?apiKey=" + API_Key;
    return makeGetReq(url);
}

/**
 * @brief Always Asynchronous
 *
 * @param channelNo
 * @param measurement
 */
void NetWorkManager::sendMeasurement(u_int8_t channelNo, const char *measurement)
{
    const char *base = API_Base;
    String url = String(base) + "feed-exp-result/insert-measurement?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo;
    sendRequest("POST", url.c_str(), measurement, channelNo, false);
}

/**
 * @brief if channel = 0 then synchronous req, otherwise asynchronous
 *
 * @param status
 * @param channelNo
 * @param rowNo
 */
void NetWorkManager::setStatus(String status, u_int8_t channelNo, u_int8_t rowNo)
{
    const char *base = API_Base;

    String url = String(base) + "feed-exp-result/set-status?apiKey=" + API_Key + "&testId=" + testId;
    bool sync = true;
    if (channelNo > 0)
    {
        url += "&channel=";
        url += channelNo;
        sync = false;
    }
    if (rowNo > 0)
    {
        url += "&row=";
        url += rowNo;
    }
    url += "&status=";
    url += status;
    sendRequest("GET", url.c_str(), "", channelNo, sync);
}

/**
 * @brief always asynchronous
 *
 * @param channelNo
 * @param rowNo
 */
void NetWorkManager::incrementMultiPlierIndex(u_int8_t channelNo, u_int8_t rowNo)
{
    const char *base = API_Base;
    String url = String(base) + "feed-exp-result/increment-multiplier-index?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo;
    if (rowNo > 0)
    {
        url += "&row=";
        url += rowNo;
    }
    sendRequest("GET", url.c_str(), "", channelNo, false);
}

char *NetWorkManager::getDriveCycle(u_int8_t channelNo, u_int8_t rowNo)
{
    const char *base = API_Base;
    String url = String(base) + "get-drive-cycle?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo + "&row=" + rowNo;
    return makeGetReq(url);
}

char *NetWorkManager::updatedUpto(u_int8_t channelNo)
{
    const char *base = API_Base;
    String url = String(base) + "feed-exp-result/updated-upto?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo;
    return makeGetReq(url);
}

bool NetWorkManager::checkAPIConnectivity()
{

    if (WiFi.status() == WL_CONNECTED)
    {
        sendRequest("GET", HOST_NAME);
        if (isPrevReqSuccess[0])
        {
            return true;
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

bool NetWorkManager::resolveResponse(String response)
{
    StaticJsonDocument<100> doc;
    DeserializationError error = deserializeJson(doc, response);

    if (error)
    {
        if (IS_LOG_ENABLED)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.f_str());
        }
        return false;
    }
    if (doc["status"] == "ok")
    {
        return true;
    }
    return false;
}

void requestCB0(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
    (void)optParm;
    if (readyState == readyStateDone)
    {
        IS_LOG_ENABLED ? Serial.println(thisRequest->responseHTTPcode()) : 0;
        if (thisRequest->responseHTTPcode() == 200 || thisRequest->responseHTTPcode() == 204)
        {
            // MODIFY The LIBRARY for
            // responseLongText() line #1062 to
            // globalLongString[lenToCopy] = 0;
            responseText_0 = thisRequest->responseLongText(); // using this takes 16KB of global memory
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("\n*************Response****************"));
                Serial.print(0);
                Serial.println(F("**************"));
                Serial.println(responseText_0);
                Serial.println(F("**************************************"));
            }
            isPrevReqSuccess[0] = true;
        }
        else
        {
            IS_LOG_ENABLED ? Serial.println(F("CH-0: Response error")) : 0;
            isPrevReqSuccess[0] = false;
            responseText_0 = emptyStr;
        }
        thisRequest->setDebug(false);
        readyToSend[0] = true;
    }
}

void requestCB1(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
    // takes almost none time
    (void)optParm;
    if (readyState == readyStateDone)
    {
        IS_LOG_ENABLED ? Serial.println(thisRequest->responseHTTPcode()) : 0;

        if (thisRequest->responseHTTPcode() == 200 || thisRequest->responseHTTPcode() == 204)
        {
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("\n*************Response-"));
                Serial.print(1);
                Serial.println(F("**************"));
                Serial.println(thisRequest->responseText());
                Serial.println(F("**************************************"));
            }
            isPrevReqSuccess[1] = true;
        }
        else
        {
            IS_LOG_ENABLED ? Serial.println(F("CH-1: Response error")) : 0;
            isPrevReqSuccess[1] = false;
        }
        thisRequest->setDebug(false);
        readyToSend[1] = true;
    }
}

void requestCB2(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
    (void)optParm;

    if (readyState == readyStateDone)
    {
        IS_LOG_ENABLED ? Serial.println(thisRequest->responseHTTPcode()) : 0;

        if (thisRequest->responseHTTPcode() == 200 || thisRequest->responseHTTPcode() == 204)
        {
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("\n*************Response-"));
                Serial.print(2);
                Serial.println(F("**************"));
                Serial.println(thisRequest->responseText());
                Serial.println(F("**************************************"));
            }
            isPrevReqSuccess[2] = true;
        }
        else
        {
            IS_LOG_ENABLED ? Serial.println(F("CH-2: Response error")) : 0;
            isPrevReqSuccess[2] = false;
        }
        thisRequest->setDebug(false);
        readyToSend[2] = true;
    }
}

void requestCB3(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
    (void)optParm;

    if (readyState == readyStateDone)
    {
        IS_LOG_ENABLED ? Serial.println(thisRequest->responseHTTPcode()) : 0;

        if (thisRequest->responseHTTPcode() == 200 || thisRequest->responseHTTPcode() == 204)
        {
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("\n*************Response-"));
                Serial.print(3);
                Serial.println(F("**************"));
                Serial.println(thisRequest->responseText());
                Serial.println(F("**************************************"));
            }
            isPrevReqSuccess[3] = true;
        }
        else
        {
            IS_LOG_ENABLED ? Serial.println(F("CH-3: Response error")) : 0;
            isPrevReqSuccess[3] = false;
        }
        readyToSend[3] = true;
    }
}

void requestCB4(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
    (void)optParm;

    if (readyState == readyStateDone)
    {
        IS_LOG_ENABLED ? Serial.println(thisRequest->responseHTTPcode()) : 0;

        if (thisRequest->responseHTTPcode() == 200 || thisRequest->responseHTTPcode() == 204)
        {
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("\n*************Response-"));
                Serial.print(4);
                Serial.println(F("**************"));
                Serial.println(thisRequest->responseText());
                Serial.println(F("**************************************"));
            }
            isPrevReqSuccess[4] = true;
        }
        else
        {
            IS_LOG_ENABLED ? Serial.println(F("CH-4: Response error")) : 0;
            isPrevReqSuccess[4] = false;
        }
        readyToSend[4] = true;
    }
}

void requestCB5(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
    (void)optParm;

    if (readyState == readyStateDone)
    {
        IS_LOG_ENABLED ? Serial.println(thisRequest->responseHTTPcode()) : 0;

        if (thisRequest->responseHTTPcode() == 200 || thisRequest->responseHTTPcode() == 204)
        {
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("\n*************Response-"));
                Serial.print(5);
                Serial.println(F("**************"));
                Serial.println(thisRequest->responseText());
                Serial.println(F("**************************************"));
            }
            isPrevReqSuccess[5] = true;
        }
        else
        {
            IS_LOG_ENABLED ? Serial.println(F("CH-5: Response error")) : 0;
            isPrevReqSuccess[5] = false;
        }
        readyToSend[5] = true;
    }
}

void requestCB6(void *optParm, AsyncHTTPRequest *thisRequest, int readyState)
{
    (void)optParm;

    if (readyState == readyStateDone)
    {
        IS_LOG_ENABLED ? Serial.println(thisRequest->responseHTTPcode()) : 0;

        if (thisRequest->responseHTTPcode() == 200 || thisRequest->responseHTTPcode() == 204)
        {
            if (IS_LOG_ENABLED)
            {
                Serial.print(F("\n*************Response-"));
                Serial.print(6);
                Serial.println(F("**************"));
                Serial.println(thisRequest->responseText());
                Serial.println(F("**************************************"));
            }
            isPrevReqSuccess[6] = true;
        }
        else
        {
            IS_LOG_ENABLED ? Serial.println(F("CH-6: Response error")) : 0;
            isPrevReqSuccess[6] = false;
        }
        readyToSend[6] = true;
    }
}

/**
 * @brief send http request asynchronous or synchronously
 * @param method "GET"|"POST"
 * @param url
 * @param body
 * @param reqChannel
 * @param isSynchronous
 */
void NetWorkManager::sendRequest(const char *method, const char *url, const char *body, uint8_t reqChannel, bool isSynchronous)
{
    isPrevReqSuccess[reqChannel] = false;
    if (!readyToSend[reqChannel])
    {
        IS_LOG_ENABLED ? Serial.println(F("Network Channel Busy.")) : 0;
        return;
    }
    bool requestOpenRes = false;
    requestOpenRes = request[reqChannel].open(method, url);
    if (requestOpenRes)
    {
        if (strlen(body) > 0)
        {
            // Serial.println(body);
            request[reqChannel].setReqHeader("Content-Type", "application/json");
            request[reqChannel].send(body);
        }
        else
        {
            request[reqChannel].send();
        }
        readyToSend[reqChannel] = false;
        IS_LOG_ENABLED ? Serial.println(F("Req sent.")) : 0;
    }
    else
    {
        IS_LOG_ENABLED ? Serial.println(F("Req sent failed!")) : 0;
        readyToSend[reqChannel] = true;
        return;
    }
    if (isSynchronous)
    {
        // wait until the req get resolved
        while (true)
        {
            // i don't know why there has to be delay to be added here to make it work
            delay(10);
            if (readyToSend[reqChannel])
                break;
        }
    }
}
