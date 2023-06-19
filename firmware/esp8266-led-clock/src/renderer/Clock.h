#pragma once

#include <LedMatrix.h>
#include <Ticker.h>
#include "Renderer.h"
#include "../TimeSource.h"

String M_arr[12] = {"Jan.", "Feb.", "Mar.", "Apr.", "May", "June", "July", "Aug.", "Sep.", "Oct.", "Nov.", "Dec."};
String WD_arr[7] = {"Sun,", "Mon,", "Tue,", "Wed,", "Thu,", "Fri,", "Sat,"};

class Clock : public Renderer
{
private:
    boolean _f_updown = false;
    unsigned short _maxPosX = SCREEN_CNT * 8 - 1;
    unsigned int _dPosX = _maxPosX;
    unsigned int _zPosX = 0;
    bool _f_tckr50ms = false;

    Ticker tick;
    TimeSource *time;

public:
    Clock(LedMatrix *mx, TimeSource *time)
        : Renderer(mx), time(time){};

    void init() override;
    void display() override;
};

void __clock_ticker_callback(bool* _flag) {
    *_flag = true;
}

void Clock::init()
{
#ifdef SCREEN_SCROLLDOWN
    _f_updown = true;
#else
    _f_updown = false;
#endif
    _zPosX = _maxPosX;
    _dPosX = -8;

    tick.attach_ms(50, __clock_ticker_callback, &_f_tckr50ms);
}

void Clock::display()
{
    auto tm = timeSource.get();

    static uint8_t sec1 = 0, sec2 = 0, min1 = 0, min2 = 0, hour1 = 0, hour2 = 0;
    static uint8_t sec11 = 0, sec12 = 0, sec21 = 0, sec22 = 0;
    static uint8_t min11 = 0, min12 = 0, min21 = 0, min22 = 0;
    static uint8_t hour11 = 0, hour12 = 0, hour21 = 0, hour22 = 0;
    static time_t lastsec{0};
    static signed int x = 0; // x1,x2;
    static signed int y = 0, y1 = 0, y2 = 0;
    static unsigned int sc1 = 0, sc2 = 0, sc3 = 0, sc4 = 0, sc5 = 0, sc6 = 0;
    static uint16_t sctxtlen = 0;
    static boolean f_scrollend_y = false;
    static boolean f_scroll_x1 = false;
    static boolean f_scroll_x2 = false;

    if (_f_updown == false)
    {
        y2 = -9;
        y1 = 8;
    }

    if (_f_updown == true)
    { // scroll  up to down
        y2 = 8;
        y1 = -8;
    }

    if (tm.tm_sec != lastsec)
    {
        lastsec = tm.tm_sec;

        sec1 = (tm.tm_sec % 10);
        sec2 = (tm.tm_sec / 10);
        min1 = (tm.tm_min % 10);
        min2 = (tm.tm_min / 10);
#ifdef SCREEN_FORMAT24H
        hour1 = (tm.tm_hour % 10); // 24 hour format
        hour2 = (tm.tm_hour / 10);
#else
        uint8_t h = tm.tm_hour; // convert to 12 hour format
        if (h > 12)
            h -= 12;
        if (h == 0)
            h = 12;
        hour1 = (h % 10);
        hour2 = (h / 10);
#endif

        y = y2; // scroll updown
        sc1 = 1;
        sec1++;
        if (sec1 == 10)
        {
            sc2 = 1;
            sec2++;
            sec1 = 0;
        }
        if (sec2 == 6)
        {
            min1++;
            sec2 = 0;
            sc3 = 1;
        }
        if (min1 == 10)
        {
            min2++;
            min1 = 0;
            sc4 = 1;
        }
        if (min2 == 6)
        {
            hour1++;
            min2 = 0;
            sc5 = 1;
        }
        if (hour1 == 10)
        {
            hour2++;
            hour1 = 0;
            sc6 = 1;
        }
#ifdef SCREEN_FORMAT24H
        if ((hour2 == 2) && (hour1 == 4))
        {
            hour1 = 0;
            hour2 = 0;
            sc6 = 1;
        }
#else
        if ((hour2 == 1) && (hour1 == 3))
        { // 12 hour format
            hour1 = 1;
            hour2 = 0;
            sc6 = 1;
        }
#endif
        sec11 = sec12;
        sec12 = sec1;
        sec21 = sec22;
        sec22 = sec2;
        min11 = min12;
        min12 = min1;
        min21 = min22;
        min22 = min2;
        hour11 = hour12;
        hour12 = hour1;
        hour21 = hour22;
        hour22 = hour2;
        if (tm.tm_sec == 45)
            f_scroll_x1 = true; // scroll ddmmyy
#ifdef UDTXT
        if (tm.tm_sec == 25)
            f_scroll_x2 = true; // scroll userdefined text
#endif
    }

    if (_f_tckr50ms == true)
    {
        _f_tckr50ms = false;
        // -------------------------------------
        if (f_scroll_x1 == true)
        {
            _zPosX++;
            _dPosX++;
            if (_dPosX == sctxtlen)
                _zPosX = 0;
            if (_zPosX == _maxPosX)
            {
                f_scroll_x1 = false;
                _dPosX = -8;
            }
        }
        // -------------------------------------
        if (f_scroll_x2 == true)
        {
            _zPosX++;
            _dPosX++;
            if (_dPosX == sctxtlen)
                _zPosX = 0;
            if (_zPosX == _maxPosX)
            {
                f_scroll_x2 = false;
                _dPosX = -8;
            }
        }

#if (SCREEN_CNT > 4)
        if (sc1 == 1)
        {
            if (_f_updown == 1)
                y--;
            else
                y++;
            mx->char2Arr_t(48 + sec12, _zPosX - 42, y);
            mx->char2Arr_t(48 + sec11, _zPosX - 42, y + y1);
            if (y == 0)
            {
                sc1 = 0;
                f_scrollend_y = true;
            }
        }
        else
            mx->char2Arr_t(48 + sec1, _zPosX - 42, 0);

        if (sc2 == 1)
        {
            mx->char2Arr_t(48 + sec22, _zPosX - 36, y);
            mx->char2Arr_t(48 + sec21, _zPosX - 36, y + y1);
            if (y == 0)
                sc2 = 0;
        }
        else
            mx->char2Arr_t(48 + sec2, _zPosX - 36, 0);

        mx->char2Arr_t(tm.tm_sec % 2 == 0 ? ':' : ';', _zPosX - 32, 0);
#endif

        if (sc3 == 1)
        {
            mx->char2Arr_t(48 + min12, _zPosX - 25, y);
            mx->char2Arr_t(48 + min11, _zPosX - 25, y + y1);
            if (y == 0)
                sc3 = 0;
        }
        else
            mx->char2Arr_t(48 + min1, _zPosX - 25, 0);

        if (sc4 == 1)
        {
            mx->char2Arr_t(48 + min22, _zPosX - 19, y);
            mx->char2Arr_t(48 + min21, _zPosX - 19, y + y1);
            if (y == 0)
                sc4 = 0;
        }
        else
            mx->char2Arr_t(48 + min2, _zPosX - 19, 0);

        mx->char2Arr_t(tm.tm_sec % 2 == 0 ? ':' : ';', _zPosX - 15 + x, 0);

        if (sc5 == 1)
        {
            mx->char2Arr_t(48 + hour12, _zPosX - 8, y);
            mx->char2Arr_t(48 + hour11, _zPosX - 8, y + y1);
            if (y == 0)
                sc5 = 0;
        }
        else
            mx->char2Arr_t(48 + hour1, _zPosX - 8, 0);

        if (sc6 == 1)
        {
            mx->char2Arr_t(48 + hour22, _zPosX - 2, y);
            mx->char2Arr_t(48 + hour21, _zPosX - 2, y + y1);
            if (y == 0)
                sc6 = 0;
        }
        else
            mx->char2Arr_t(48 + hour2, _zPosX - 2, 0);

        if (f_scroll_x1)
        { // day month year
            String txt = "   ";
            txt += WD_arr[tm.tm_wday] + " ";
            txt += String(tm.tm_mday) + ". ";
            txt += M_arr[tm.tm_mon] + " ";
            // txt += String(tm.tm_year + 1900) + "   ";
            sctxtlen = mx->scrollText(_dPosX, txt);
        }
        //      -------------------------------------
        if (f_scroll_x2)
        { // user defined text
#ifdef UDTXT
            sctxtlen = scrolltext(_dPosX, UDTXT);
#endif // UDTXT
        }

        mx->apply();
        if (f_scrollend_y == true)
            f_scrollend_y = false;
    } // end 50ms

    // -----------------------------------------------
    if (y == 0)
    {
        // do something else
    }
}