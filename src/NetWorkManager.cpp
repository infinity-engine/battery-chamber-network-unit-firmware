#include "NetWorkManager.h"

NetWorkManager::NetWorkManager() {}

bool NetWorkManager::setup()
{
    WiFi.mode(WIFI_STA);
    while (!wifi.addAP(WiFi_SSID, WiFi_PWD))
    {
        IS_LOG_ENABLED ? Serial.println("Trying to connect to the WiFi ") : 0;
        delay(1000);
    }
    Serial.println(F("WiFi connection success!"));
    while (!checkInternetConnectivity())
    {

        IS_LOG_ENABLED ? Serial.println(F("WiFi doesn't have internet access.")) : 0;
        delay(1000);
    }
    IS_LOG_ENABLED ? Serial.println(F("WiFi has internet connectivity.")) : 0;
    testId = "6422d33bccf18704bc0ef835";
    return true;
}

String NetWorkManager::makePostReq(String url, String jsonString)
{
    // on average taking around 500ms-1s max
    String response = "";
    if (wifi.run() == WL_CONNECTED)
    {
        if (http.begin(client, url))
        {
            http.addHeader("Content-Type", "application/json");
            int httpCode = http.POST(jsonString);
            if (httpCode > 0)
            {
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED)
                {
                    response = http.getString();
                }
            }
            else
            {
                IS_LOG_ENABLED ? Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str()) : 0;
            }
        }
        http.end();
    }
    return response;
}

String NetWorkManager::makeGetReq(String url)
{
    String response = "";
    if (wifi.run() == WL_CONNECTED)
    {
        if (http.begin(client, url))
        {
            int httpCode = http.GET();
            if (httpCode > 0)
            {
                // httpcode will be negative on error
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    response = http.getString();
                }
            }
            else
            {
                IS_LOG_ENABLED ? Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str()) : 0;
            }
        }
        http.end();
    }

    return response;
}

void NetWorkManager::fetchExp()
{
    const char *base = API_Base;
    String url = String(base) + "is-any-experiment?apiKey=" + API_Key;
    Serial.println(makeGetReq(url));
}

bool NetWorkManager::sendMeasurement(u_int8_t channelNo, String measurement)
{

    const char *base = API_Base;
    String url = String(base) + "feed-exp-result/insert-measurement?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo;
    return resolveResponse(makePostReq(url, measurement));
}

bool NetWorkManager::setStatus(String status, u_int8_t channelNo, u_int8_t rowNo)
{
    const char *base = API_Base;

    String url = String(base) + "feed-exp-result/set-status?apiKey=" + API_Key + "&testId=" + testId;
    if (channelNo > 0)
    {
        url += +"&channel=" + channelNo;
    }
    if (rowNo > 0)
    {
        url += "&row=" + rowNo;
    }
    url += "&status=" + status;
    return resolveResponse(makeGetReq(url));
}

bool NetWorkManager::incrementMultiPlierIndex(u_int8_t channelNo, u_int8_t rowNo)
{
    const char *base = API_Base;
    String url = String(base) + "feed-exp-result/increment-multiplier-index?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo;
    if (rowNo > 0)
    {
        url += "&row=" + rowNo;
    }
    return resolveResponse(makeGetReq(url));
}

String NetWorkManager::getDriveCycle(u_int8_t channelNo, u_int8_t rowNo)
{
    const char *base = API_Base;
    String url = String(base) + "get-drive-cycle?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo + "&row=" + rowNo;
    return makeGetReq(url);
}

String NetWorkManager::updatedUpto(u_int8_t channelNo)
{
    const char *base = API_Base;
    String url = String(base) + "feed-exp-result/updated-upto?apiKey=" + API_Key + "&testId=" + testId + "&channel=" + channelNo;
    return makeGetReq(url);
}

bool NetWorkManager::checkInternetConnectivity()
{

    if (wifi.run() == WL_CONNECTED)
    {
        http.begin(client, "http://clients3.google.com/generate_204");

        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_NO_CONTENT)
        {
            http.end();
            return true;
        }
        else
        {
            http.end();
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