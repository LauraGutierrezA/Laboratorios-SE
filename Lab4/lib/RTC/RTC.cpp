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
    uint8_t data[7];
    for (int i = 0; i < 7; i++) data[i] = time[i];
    // Bit 7 del registro de segundos es CH (Clock Halt): 0 = oscilador activo.
    // DS1307 arranca con CH indefinido cuando Vbat=GND → forzamos CH=0 explícitamente.
    data[0] &= 0x7F;
    _i2c_rtc.writeBytes(_addr, 0x00, data, 7);
}

bool Rtc::getTime(uint8_t &hour, uint8_t &min, uint8_t &sec){
    uint8_t datos[7];
    if (!_i2c_rtc.readBytes(_addr, 0x00, datos, 7)) return false;

    // Si CH=1 (bit 7 del registro de segundos) el oscilador está detenido.
    // Intentamos relanzarlo escribiendo CH=0 sin tocar los demás registros.
    if (datos[0] & 0x80) {
        datos[0] &= 0x7F;
        _i2c_rtc.writeBytes(_addr, 0x00, datos, 1);
        return false;
    }

    sec  = bcdaDecimal(datos[0] & 0x7F);
    min  = bcdaDecimal(datos[1] & 0x7F);
    hour = bcdaDecimal(datos[2] & 0x3F);
    return true;
}
