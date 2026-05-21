#include "RTC.h"


Rtc::Rtc(I2CMaster i2c_rtc, uint8_t addr) {
    _i2c_rtc = i2c_rtc;
    _addr = addr;
}

Rtc::~Rtc(){

}

uint8_t Rtc::decimalaBcd (uint8_t decimal){
    return(((decimal/10) << 4) | (decimal % 10)); 
}

uint8_t Rtc::bcdaDecimal (uint8_t bcd){
    return((bcd >> 4)*10 + (bcd & 0x0F)); 
}

void Rtc::setTime(uint8_t time[7]){
    _i2c_rtc.writeBytes(_addr, 0x00, time, 7);
}

void Rtc::getTime(uint8_t &hour, uint8_t &min, uint8_t &sec){
    uint8_t datos[7];
    if(_i2c_rtc.readBytes(_addr, 0x00, datos, 7)){
        sec = bcdaDecimal((datos[0] & 0x7F)); 
        min = bcdaDecimal((datos[1] & 0x7F)); 
        hour = bcdaDecimal((datos[2] & 0x3F)); 
    }; 

}
