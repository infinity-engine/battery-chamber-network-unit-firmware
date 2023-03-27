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
    return true;
}

String NetWorkManager::makePostReq(String url)
{
    if (wifi.run() == WL_CONNECTED)
    {
    }
    return "true";
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
    Serial.println(url);
    Serial.println(makeGetReq(url));
}

void NetWorkManager::test()
{
    // wait for WiFi connection
    if ((wifi.run() == WL_CONNECTED))
    {

        Serial.print("[HTTP] begin...\n");
        if (http.begin(client, "http://jigsaw.w3.org/HTTP/connection.html"))
        { // HTTP

            Serial.print("[HTTP] GET...\n");
            // start connection and send HTTP header
            int httpCode = http.GET();

            // httpCode will be negative on error
            if (httpCode > 0)
            {
                // HTTP header has been send and Server response header has been handled
                Serial.printf("[HTTP] GET... code: %d\n", httpCode);

                // file found at server
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    String payload = http.getString();
                    Serial.println(payload);
                }
            }
            else
            {
                Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
            }

            http.end();
        }
        else
        {
            Serial.printf("[HTTP} Unable to connect\n");
        }
    }
}