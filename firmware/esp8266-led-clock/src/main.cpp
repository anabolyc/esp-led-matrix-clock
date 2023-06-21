#include <Arduino.h>
#include <SPI.h>

#include <LedMatrix.h>
LedMatrix matrix(SCREEN_CNT);

#include "renderer/Loader.h"
Loader loader(&matrix);

#include <WiFiManager.h>
WiFiManager wm;
bool __config_portal_running = false;

#include "TimeSource.h"
TimeSource timeSource(&loader, &wm);

#include "renderer/Clock.h"
Clock _clock(&matrix, &timeSource);

#include "renderer/AutoBrightness.h"
AutoBrightness Brightness(&matrix, &timeSource);

#include "renderer/Renderer.h"
Renderer *renderers[] = {
    &Brightness,
    &_clock};

void configPortalCallback(WiFiManager *wm)
{
    Serial.printf("Config portal started on %s AP\n", wm->getWiFiHostname().c_str());
    loader.setText("Connect to " + wm->getWiFiHostname() + " to configure WiFi");
    __config_portal_running = true;
}

void saveConfigCallback()
{
    Serial.print("Config portal stopped\n");
    __config_portal_running = false;
    if (timeSource.sync())
    {
        loader.stop();
        matrix.clear();
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD);
    Serial.setDebugOutput(true);
#if ARDUINO_HW_CDC_ON_BOOT
    delay(3000);
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

#ifdef ARDUINO_LOLIN_C3_MINI
    // https://github.com/tzapu/WiFiManager/issues/1422
    Serial.println("Set WIFI_POWER_8_5dBm");
    WiFi.setTxPower(WIFI_POWER_8_5dBm); 
#endif

    // For purposes DEBUG only
    // wm.resetSettings();
    
    wm.setConnectTimeout(10);
    wm.setConfigPortalBlocking(false);
#if defined(WIFI_SSID) && defined(WIFI_PASS)
    wm.preloadWiFi(WIFI_SSID, WIFI_PASS);
#endif
    wm.setAPCallback(configPortalCallback);
    wm.setSaveConfigCallback(saveConfigCallback);

    if (timeSource.sync())
    {
        loader.stop();
        matrix.clear();
    }
}

void loop()
{
    if (__config_portal_running) {
        wm.process();
    }
    else
    {
        for (uint8_t i = 0; i < sizeof(renderers) / sizeof(Renderer *); i++)
        {
            renderers[i]->display();
        }
        timeSource.loop();
    }
}