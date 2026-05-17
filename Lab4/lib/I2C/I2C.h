#ifndef __I2C_MASTER_H__
#define __I2C_MASTER_H__

#include "driver/i2c.h"

class I2CMaster {
    private:
        i2c_port_t _port; 
        int _sdaPin; 
        int _sclPin; 
        uint32_t _speedHz; 
    public:
        I2CMaster(i2c_port_t port, int sdaPin, int sclPin, uint32_t speedHz = 100000);
        void init(); 
        bool writeBytes(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, size_t length); 
        bool readBytes(uint8_t slaveAddr, uint8_t regAddr, uint8_t *data, size_t length); 
        ~I2CMaster();
};





#endif