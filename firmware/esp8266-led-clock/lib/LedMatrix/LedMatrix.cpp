#include <Arduino.h>
#include <SPI.h>
#include "LedMatrix.h"
#include "fonts.h"

const uint8_t init_seq_length = 7;
const uint8_t InitArr[init_seq_length][2] =
    {
        {0x0C, 0x00}, // display off
        {0x00, 0xFF}, // no LEDtest
        {0x09, 0x00}, // BCD off
        {0x0F, 0x00}, // normal operation
        {0x0B, 0x07}, // start display
        {0x0A, 0x04}, // brightness
        {0x0C, 0x01}  // display on
};

const uint8_t _maxPosX = SCREEN_CNT * 8 - 1;
uint8_t _helpArrMAX[SCREEN_CNT * 8];   // helperarray for chardecoding
uint8_t _helpArrPos[SCREEN_CNT * 8];   // helperarray pos of chardecoding
uint16_t _chbuf[256];

void LedMatrix::init()
{
    pinMode(PIN_SPI_CS, OUTPUT);
    digitalWrite(PIN_SPI_CS, HIGH);

    uint8_t j = 0, k = 0;
    for (uint8_t i = 0; i < Count * 8; i++)
    {
        _helpArrPos[i] = (1 << j); 
        _helpArrMAX[i] = k;
        j++;
        if (j > 7)
        {
            j = 0;
            k++;
        }
    }

    #if defined(PIN_SPI_CLK) && defined(PIN_SPI_MISO) && defined(PIN_SPI_MOSI)
    SPI.begin(PIN_SPI_CLK, PIN_SPI_MISO, PIN_SPI_MOSI);
    #else
    SPI.begin();
    #endif

    for (uint8_t i = 0; i < init_seq_length; i++)
    {
        digitalWrite(PIN_SPI_CS, LOW);
        delayMicroseconds(1);
        for (uint8_t j = 0; j < Count; j++)
        {
            SPI.write(InitArr[i][0]); // register
            SPI.write(InitArr[i][1]); // value
        }
        digitalWrite(PIN_SPI_CS, HIGH);
    }
}

LedMatrix::~LedMatrix()
{
    // delete[] Leds;
}

void LedMatrix::invert() {
    isInvert = !isInvert;
}

void LedMatrix::setBrightness(uint8_t br)
{
    uint8_t j;
    if (br < 16)
    {
        digitalWrite(PIN_SPI_CS, LOW);
        delayMicroseconds(1);
        for (j = 0; j < Count; j++)
        {
            SPI.write(0x0A); // register
            SPI.write(br);   // value
        }
        digitalWrite(PIN_SPI_CS, HIGH);
    }
}

void LedMatrix::apply()
{
    // dumpToConsole();
    // #ifdef ROTATE_90
    //     rotate_90();
    // #endif

    for (uint8_t i = 0; i < 8; i++)
    { // 8 rows
        digitalWrite(PIN_SPI_CS, LOW);
        delayMicroseconds(1);
        for (uint8_t j = Count; j > 0; j--)
        {
            SPI.write(i + 1);
#ifdef REVERSE_HORIZONTAL
            SPI.setBitOrder(LSBFIRST); // bitorder for reverse columns
#endif

#ifdef REVERSE_VERTICAL
            SPI.write(isInvert ? ~(Leds[j - 1][7 - i]) : Leds[j - 1][7 - i]);
#else
            SPI.write(isInvert ? ~(Leds[j - 1][i]) : Leds[j - 1][i]);
#endif

#ifdef REVERSE_HORIZONTAL
            SPI.setBitOrder(MSBFIRST); // reset bitorder
#endif
        }
        digitalWrite(PIN_SPI_CS, HIGH);
    }
}

void LedMatrix::dumpToConsole()
{
    Serial.println("\nLedMatrix::dump");
    for (uint8_t y = 0; y < 8; y++)
    {
        for (uint8_t x = 0; x < Count; x++)
        {
            Serial.printf("%02x ", Leds[x][y]);
        }
        Serial.println();
    }
}

void LedMatrix::clear()
{
    memset(Leds, 0, sizeof(Leds));
    apply();
}

// void rotate_90()
// { // for Generic displays
//     for (uint8_t k = Count; k > 0; k--)
//     {
//         uint8_t i, j, m, imask, jmask;
//         uint8_t tmp[8] = {0, 0, 0, 0, 0, 0, 0, 0};
//         for (i = 0, imask = 0x01; i < 8; i++, imask <<= 1)
//         {
//             for (j = 0, jmask = 0x01; j < 8; j++, jmask <<= 1)
//             {
//                 if (Leds[k - 1][i] & jmask)
//                 {
//                     tmp[j] |= imask;
//                 }
//             }
//         }
//         for (m = 0; m < 8; m++)
//         {
//             Leds[k - 1][m] = tmp[m];
//         }
//     }
// }

uint8_t LedMatrix::char2Arr_t(unsigned short ch, int PosX, short PosY)
{ // characters into arr, shows only the time
    int i, j, k, l, m, o1, o2, o3, o4 = 0;
    PosX++;
    k = ch - 0x30; // ASCII position in font
    if ((k >= 0) && (k < 12))
    {                      // character found in font?
        o4 = font_t[k][0]; // character width
        o3 = 1 << (o4 - 1);
        for (i = 0; i < o4; i++)
        {
            if (((PosX - i <= _maxPosX) && (PosX - i >= 0)) && ((PosY > -8) && (PosY < 8)))
            { // within matrix?
                o1 = _helpArrPos[PosX - i];
                o2 = _helpArrMAX[PosX - i];
                for (j = 0; j < 8; j++)
                {
                    if (((PosY >= 0) && (PosY <= j)) || ((PosY < 0) && (j < PosY + 8)))
                    { // scroll vertical
                        l = font_t[k][j + 1];
                        m = (l & (o3 >> i)); // e.g. o4=7  0zzzzz0, o4=4  0zz0
                        if (m > 0)
                            Leds[o2][j - PosY] = Leds[o2][j - PosY] | (o1); // set point
                        else
                            Leds[o2][j - PosY] = Leds[o2][j - PosY] & (~o1); // clear point
                    }
                }
            }
        }
    }
    return o4;
}

uint8_t LedMatrix::char2Arr_p(uint16_t ch, int PosX)
{ // characters into arr, proportional font
    int i, j, l, m, o1, o2, o3, o4 = 0;
    if (ch <= 345)
    {                       // character found in font?
        o4 = font_p[ch][0]; // character width
        o3 = 1 << (o4 - 1);
        for (i = 0; i < o4; i++)
        {
            if ((PosX - i <= _maxPosX) && (PosX - i >= 0))
            { // within matrix?
                o1 = _helpArrPos[PosX - i];
                o2 = _helpArrMAX[PosX - i];
                for (j = 0; j < 8; j++)
                {
                    l = font_p[ch][j + 1];
                    m = (l & (o3 >> i)); // e.g. o4=7  0zzzzz0, o4=4  0zz0
                    if (m > 0)
                        Leds[o2][j] = Leds[o2][j] | (o1); // set point
                    else
                        Leds[o2][j] = Leds[o2][j] & (~o1); // clear point
                }
            }
        }
    }
    return o4;
}

uint16_t LedMatrix::scrollText(int16_t posX, String txt)
{
    uint16_t i = 0, j = 0;
    boolean k = false;
    while ((txt[i] != 0) && (j < 256))
    {
        if ((txt[i] >= 0x20) && (txt[i] <= 0x7f))
        { // ASCII section
            _chbuf[j] = txt[i] - 0x20;
            k = true;
            i++;
            j++;
        }
        if (txt[i] == 0xC2)
        { // basic latin section (0x80...0x9f are controls, not used)
            if ((txt[i + 1] >= 0xA0) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] - 0x40;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xC3)
        { // latin1 supplement section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] + 0x00;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xCE)
        { // greek section
            if ((txt[i + 1] >= 0x91) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] + 0x2F;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xCF)
        { // greek section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0x89))
            {
                _chbuf[j] = txt[i + 1] + 0x6F;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xD0)
        { // cyrillic section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0xBF))
            {
                _chbuf[j] = txt[i + 1] + 0x79;
                k = true;
                i += 2;
                j++;
            }
        }
        if (txt[i] == 0xD1)
        { // cyrillic section
            if ((txt[i + 1] >= 0x80) && (txt[i + 1] <= 0x9F))
            {
                _chbuf[j] = txt[i + 1] + 0xB9;
                k = true;
                i += 2;
                j++;
            }
        }
        if (k == false)
        {
            _chbuf[j] = 0x00; // space 1px
            i++;
            j++;
        }
        k = false;
    }
    //  _chbuf stores the position of th char in font and in j is the length of the real string

    int16_t p = 0;
    for (int k = 0; k < j; k++)
    {
        p += char2Arr_p(_chbuf[k], posX - p);
        p += char2Arr_p(0, posX - p); // 1px space
        if (_chbuf[k] == 0)
            p += 2; // +2px space
    }
    return p;
}