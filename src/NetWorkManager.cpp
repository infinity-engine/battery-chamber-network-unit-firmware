#include "NetWorkManager.h"

NetWorkManager::NetWorkManager()
{
}

bool NetWorkManager::setup()
{
    WiFi.mode(WIFI_STA);
    while (!wifi.addAP(WiFi_SSID, WiFi_PWD))
    {
        Serial.println("Trying to connect to the WiFi ");
        delay(1000);
    }
    Serial.println(F("WiFi connection success!"));
    while (!checkInternetConnectivity())
    {
        Serial.println(F("WiFi doesn't have internet access."));
        delay(1000);
    }
    Serial.println(F("WiFi has internet connectivity."));
    testId = "641ef4097c489a59f1c8e248";
    return true;
}

String NetWorkManager::makePostReq(String url, String jsonString)
{
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
                    String response = http.getString();
                }
            }
            else
            {
                Serial.printf("[HTTP] POST failed, error: %s\n", http.errorToString(httpCode).c_str());
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
                Serial.print(F("Request failed with "));
                Serial.println(httpCode);
                Serial.println(http.errorToString(httpCode));
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
    Serial.println(url);
    Serial.println(makePostReq(url, measurement));//to check not printing
    return true;
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
