#pragma once

#include "Renderer.h"
#include <Ticker.h>

const String __loader_string = "Loading...";

class Loader : public Renderer
{
private:
    Ticker tick;
    const unsigned short _maxPosX = SCREEN_CNT * 8 - 1;

public:
    unsigned int _dPosX = _maxPosX;
    Loader(LedMatrix *mx) : Renderer(mx){};

    void init() override;
    void stop();
};

void __loader_tick_callback(Loader* self) {
    self->mx->scrollText(self->_dPosX++, __loader_string);
    self->mx->apply();
}

void Loader::init()
{
    tick.attach_ms(200, __loader_tick_callback, this);
}

void Loader::stop()
{
    tick.detach();
}