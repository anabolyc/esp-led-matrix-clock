#pragma once

#include "Renderer.h"
#include <Ticker.h>

enum LoaderState : uint8_t
{
    CONNECTING,
    GET_IP,
    GET_LOCATION,
    GET_TZ,
    GET_TIME,
    READY,
    INIT
};

String __loader_string = "";

const String _loader_strings[] = {
    "Connecting",
    "Get my IP",
    "Find location",
    "Time zone",
    "Get time",
    "Ready",
    "Initializing"
};

class Loader : public Renderer
{
private:
    Ticker tick;
public:
    const unsigned short _maxPosX = SCREEN_CNT * 8 - 1;
    unsigned int _dPosX = _maxPosX;
    Loader(LedMatrix *mx) : Renderer(mx){};

    void init() override;
    void setState(LoaderState);
    void setText(String s) { __loader_string = s; _dPosX = _maxPosX; }
    void stop();
};

void __loader_tick_callback(Loader* self) {
    self->mx->scrollText((self->_dPosX++) % (self->_maxPosX + __loader_string.length() * 8), __loader_string);
    self->mx->apply();
}

void Loader::init()
{
    tick.attach_ms(200, __loader_tick_callback, this);
    __loader_string = _loader_strings[INIT];
}

void Loader::setState(LoaderState state) {
    __loader_string = _loader_strings[state];
    _dPosX = _maxPosX;
};

void Loader::stop()
{
    tick.detach();
}