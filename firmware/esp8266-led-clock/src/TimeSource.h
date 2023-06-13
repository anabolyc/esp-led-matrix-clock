#pragma once

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
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

#define NTP_UPDATE_EVERY_SEC (60 * 60 * 8)

const String _my_ip_url = "https://api.my-ip.io/ip";

class TimeSource
{
private:
    Loader *_loader;
    WiFiManager *_wifiManager;
    Ticker tick;
    tm Time;

    String getIpAddress();

public:
    TimeSource(Loader *loader) : _loader(loader), _wifiManager(nullptr){};
    TimeSource(Loader *loader, WiFiManager *wm) : _loader(loader), _wifiManager(wm){};
    ~TimeSource();

    void init();
    void sync();

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
    self->sync();
}

void TimeSource::init()
{
    tick.attach(NTP_UPDATE_EVERY_SEC, __timesource_tick_callback, this);
}

String TimeSource::getIpAddress()
{
    WiFiClientSecure client;
    client.setInsecure();

    HTTPClient http;
    String result = "";

    if (http.begin(client, _my_ip_url))
    { // HTTP

        // Serial.print("[HTTP] GET...\n");
        // start connection and send HTTP header
        int httpCode = http.GET();

        // httpCode will be negative on error
        if (httpCode > 0)
        {
            // HTTP header has been send and Server response header has been handled
            // Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
            {
                String payload = http.getString();
                result = payload;
            }
        }
        // else
        // {
        //     Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        // }

        http.end();
    }
    // else
    // {
    //     Serial.printf("[HTTP} Unable to connect\n");
    // }

    return result;
}

void TimeSource::sync()
{
    _loader->setState(Loader::CONNECTING);

    WiFi.mode(WIFI_STA);
#ifdef ARDUINO_LOLIN_C3_MINI
    WiFi.setTxPower(WIFI_POWER_8_5dBm); // https://github.com/tzapu/WiFiManager/issues/1422
#endif

    if (!_wifiManager)
    {
        uint8_t cnt = 0;
#if defined(WIFI_SSID) && defined(WIFI_PASS)
        WiFi.begin(WIFI_SSID, WIFI_PASS);
#endif
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
            cnt++;
            if (cnt > 20)
                break;
        }

        if (WiFi.status() != WL_CONNECTED)
            return;
    }
    else if (!_wifiManager->autoConnect())
    {
        Serial.println("Failed to connect and hit timeout");
#ifdef ESP8266
        ESP.reset();
#endif
#ifdef ESP32
        ESP.restart();
#endif
    }

    Serial.println("\nConnected with: " + WiFi.SSID());
    Serial.println("Private IP Address: " + WiFi.localIP().toString());

    // loader->setState(GET_IP);
    String ip = getIpAddress();
    Serial.println("Public IP Address: " + ip);

    // loader->setState(GET_TIME);
    time_t now;

#ifdef ESP8266
    configTime(TIMEZONE, NTP_SERVERS);
#endif
#ifdef ESP32
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVERS);
#endif

    Serial.print("Requesting current time ");
    int i = 1;
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

    WiFi.mode(WIFI_OFF);
}
