/*
ESTO EN EL MAIN SE HARÍA: 
I2CMaster i2cBus(I2C_NUM_0, 21, 22, 100000);
i2cBus.init();

Rtc miReloj(i2cBus, 0x68);

uint8_t h, m, s;
miReloj.getTime(h, m, s);

*/

#ifndef __RTC_H__
#define __RTC_H__

#include "I2C.h"
#include "esp_log.h"

class Rtc {
    private: 
        I2CMaster _i2c_rtc;
        uint8_t _addr;

    public: 
        Rtc(I2CMaster rtc, uint8_t addr);
        ~Rtc();
        void setTime(uint8_t time[7]);
        bool getTime(uint8_t &hour, uint8_t &min, uint8_t &sec);
        uint8_t decimalaBcd (uint8_t decimal);
        uint8_t bcdaDecimal (uint8_t bcd);


};


#endif