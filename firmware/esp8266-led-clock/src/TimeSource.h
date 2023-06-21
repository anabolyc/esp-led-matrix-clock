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
#include <time.h>
#include "tz.h"
#include "renderer/Loader.h"

#define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
#define NTP_MIN_VALID_EPOCH 1533081600 // August 1st, 2018

#define NTP_UPDATE_EVERY_SEC (60 * 60 * 2) // Every 2 hrs

const String _my_ip_url = "https://api.my-ip.io/ip";
const String _my_timezone_url = "http://ip-api.com/line/?fields=timezone";
const String _my_timeoffset_url = "http://ip-api.com/line/?fields=offset";

class TimeSource
{
private:
    Loader *_loader;
    WiFiManager *_wifiManager;
    Ticker tick;
    tm Time;

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
            Serial.printf("[HTTP] GET failed, error: %s\n", http.errorToString(httpCode).c_str());
            return false;
        }

        http.end();
    }
    else
    {
        Serial.printf("[HTTP} Unable to connect\n");
        return false;
    }

    return true;
}

bool TimeSource::sync()
{
    if (!connect())
        return false;

    // _loader->setState(GET_IP);
    // String ip = "";
    // if (getUrl(_my_ip_url, ip))
    // {
    //     Serial.println("Public IP Address: " + ip);
    // }

    _loader->setState(GET_TZ);
    String tz = "0";
    if (getUrl(_my_timeoffset_url, tz))
    {
        Serial.printf("Timezone offset: %s\n", tz);
    }
    configTime(atoi(tz.c_str()), 0, NTP_SERVERS);

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
