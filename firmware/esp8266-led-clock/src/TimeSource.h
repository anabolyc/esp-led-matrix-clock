#pragma once

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#endif
#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#endif

#include <WiFiManager.h>
#include <Ticker.h>
#include <time.h>
#include "tz.h"

#define NTP_SERVERS "us.pool.ntp.org", "time.nist.gov", "pool.ntp.org"
#define NTP_MIN_VALID_EPOCH 1533081600 // August 1st, 2018

#define NTP_UPDATE_EVERY_SEC (60 * 60 * 24)

class TimeSource
{
private:
    WiFiManager *_wifiManager;
    Ticker tick;
    tm Time;

public:
    TimeSource() : _wifiManager(nullptr){};
    TimeSource(WiFiManager *wm) : _wifiManager(wm){};
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

void TimeSource::sync()
{
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
    Serial.println("IP Address: " + WiFi.localIP().toString());

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
