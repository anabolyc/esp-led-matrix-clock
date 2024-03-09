#pragma once

#include <Arduino.h>

#ifdef ESP32
#include <WiFi.h>
#include <HTTPClient.h>
#endif

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#endif

#include <WiFiManager.h>
#include <WiFiClientSecure.h>
#include <Ticker.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <time.h>
#include "tz.h"
#include "renderer/Loader.h"

#define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
#define NTP_MIN_VALID_EPOCH 1533081600 // August 1st, 2018

#define NTP_UPDATE_EVERY_SEC (60 * 60 * 2) // Every 2 hrs

const String _my_ip_url = "https://api.my-ip.io/ip";
const String _my_json_url = "http://ip-api.com/json?fields=status,offset";

#define EEPROM_MAGIC_NUMBER 0x12

struct SavedSettings
{
    uint32_t tz_offset;
};

class TimeSource
{
private:
    Loader *_loader;
    WiFiManager *_wifiManager;
    Ticker tick;
    tm Time;
    SavedSettings settings;

    WiFiClientSecure _secureclient;
    WiFiClient _client;
    bool _sync_scheduled = false;

    bool connect();
    void disconnect();
    bool getUrl(const String, String &);

public:
    TimeSource(Loader *loader, WiFiManager *wm) : _loader(loader), _wifiManager(wm){};
    ~TimeSource();

    void scheduleSync() { _sync_scheduled = true; };
    void init();
    bool sync();
    void loop()
    {
        if (_sync_scheduled)
        {
            sync();
            _sync_scheduled = false;
        }
    };

    tm get();
};

tm TimeSource::get()
{
    time_t now = time(&now);
    localtime_r(&now, &Time);
    return Time;
}

TimeSource::~TimeSource()
{
    tick.detach();
}

void __timesource_tick_callback(TimeSource *self)
{
    self->scheduleSync();
}

bool TimeSource::connect()
{
    _loader->setState(CONNECTING);

    if (_wifiManager->autoConnect())
    {
        Serial.println("\nConnected with: " + WiFi.SSID());
        Serial.println("Private IP Address: " + WiFi.localIP().toString());
        return true;
    }
    {
        Serial.println("Failed to connect");
        return false;
    }
}

void TimeSource::disconnect()
{
    _wifiManager->disconnect();
}

void TimeSource::init()
{
    tick.attach(NTP_UPDATE_EVERY_SEC, __timesource_tick_callback, this);
    EEPROM.begin(1 + sizeof(SavedSettings));
}

bool TimeSource::getUrl(const String url, String &result)
{
    WiFiClient client = this->_client;
    if (url.startsWith("https"))
    {
        this->_secureclient.setInsecure();
        client = this->_secureclient;
    }

    HTTPClient http;

    for (uint tries = 0; tries < 3; tries++)
    {
        if (http.begin(client, url))
        { // HTTP

            Serial.printf("[HTTP] GET %s\n", url.c_str());
            // start connection and send HTTP header
            int httpCode = http.GET();

            // httpCode will be negative on error
            if (httpCode > 0)
            {
                // HTTP header has been send and Server response header has been handled
                Serial.printf("[HTTP] GET %d code\n", httpCode);

                // file found at server
                if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
                {
                    String payload = http.getString();
                    // Serial.printf("[HTTP] GET %s\n", payload.c_str());
                    result = payload;
                }
                else
                {
                    return false;
                }
            }
            else
            {
                Serial.printf("[HTTP] (%d of %d) GET failed, error: %s\n", tries + 1, 3, http.errorToString(httpCode).c_str());
            }

            http.end();
        }
        else
        {
            Serial.printf("[HTTP] (%d of %d) Unable to connect\n", tries + 1, 3);
        }
    }

    return true;
}

bool TimeSource::sync()
{
    if (!connect())
        return false;

    _loader->setState(GET_TZ);

    #ifndef TZ_OFFSET
    String response = "";
    if (getUrl(_my_json_url, response))
    {
        Serial.printf("Response: %s\n", response.c_str());
        DynamicJsonDocument doc(256);
        deserializeJson(doc, response);
        const char *status = doc["status"];
        if (String(status) == "success")
        {
            settings.tz_offset = doc["offset"];
            EEPROM.put(0, EEPROM_MAGIC_NUMBER);
            EEPROM.put(1, settings);
            EEPROM.commit();
        }
        else
        {
            Serial.print("Failed to get TZ offset, try restoring from saved state\n");
            if (EEPROM.read(0) == EEPROM_MAGIC_NUMBER)
                EEPROM.get(1, settings);
            else
                Serial.print("No state is saved in EEPROM, falling back to UTC\n");
        }
    }
    else
    {
        Serial.print("Failed to get TZ offset, restoring from saved state\n");
        if (EEPROM.read(0) == EEPROM_MAGIC_NUMBER)
            EEPROM.get(1, settings);
        else
            Serial.print("No state is saved in EEPROM, falling back to UTC\n");
    }

    Serial.printf("Timezone offset: %d\n", settings.tz_offset);
    configTime(settings.tz_offset, 0, NTP_SERVERS);
    #else
    Serial.printf("Timezone offset (hardwired): %d\n", TZ_OFFSET);
    configTime(TZ_OFFSET, 0, NTP_SERVERS);
    #endif
    

    _loader->setState(GET_TIME);
    Serial.print("Requesting current time...");

    int i = 1;
    time_t now;
    while ((now = time(nullptr)) < NTP_MIN_VALID_EPOCH)
    {
        Serial.print(".");
        delay(300);
        yield();
        i++;
    }
    Serial.println("Time synchronized");
    Serial.printf("Local time: %s", asctime(localtime(&now))); // print formated local time, same as ctime(&now)
    Serial.printf("UTC time:   %s", asctime(gmtime(&now)));    // print formated GMT/UTC time

    disconnect();
    return true;
}
