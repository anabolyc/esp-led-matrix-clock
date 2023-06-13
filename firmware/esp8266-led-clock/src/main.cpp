#include <Arduino.h>
#include <SPI.h>

#include <LedMatrix.h>
LedMatrix matrix(SCREEN_CNT);

#include "renderer/Loader.h"
Loader loader(&matrix);

#include "TimeSource.h"
#if defined(WIFI_SSID) && defined(WIFI_PASS)
TimeSource timeSource(&loader);
#else
#include <WiFiManager.h>
WiFiManager wm;
TimeSource timeSource(&loader, &wm);
#endif

#include "renderer/Clock.h"
Clock _clock(&matrix, &timeSource);

#include "renderer/AutoBrightness.h"
AutoBrightness Brightness(&matrix, &timeSource);

#include "renderer/Renderer.h"
Renderer *renderers[] = {
    &Brightness,
    &_clock
};

void setup()
{
    Serial.begin(SERIAL_BAUD);
#if ARDUINO_HW_CDC_ON_BOOT
    delay(2000);
#else
    delay(100);
#endif

    timeSource.init();
    matrix.init();
    matrix.clear();
    #ifdef SCREEN_INVERT
    matrix->invert();
    #endif
    
    loader.init();

    for (uint8_t i = 0; i < sizeof(renderers) / sizeof(Renderer *); i++)
    {
        renderers[i]->init();
    }

    WiFi.mode(WIFI_OFF);
    String ssid = "ESP-" + String((unsigned long)
#ifdef ESP8266
        ESP.getChipId()
#endif
#ifdef ESP32
        ESP.getEfuseMac()
#endif
    );
    WiFi.hostname(ssid);

    timeSource.sync();
    loader.stop();
    matrix.clear();
    Serial.println("Setup finished!");
}

void loop()
{
    for (uint8_t i = 0; i < sizeof(renderers) / sizeof(Renderer *); i++)
    {
        renderers[i]->display();
    }
}

// TODO: sync Time every T