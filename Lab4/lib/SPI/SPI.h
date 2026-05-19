#ifndef __SPI_H__
#define __SPI_H__

#include "driver/spi_master.h"
#include "driver/gpio.h"

class SPI {
    private:
        spi_host_device_t _spiHost;
        int _mosiPin;
        int _misoPin;
        int _clkPin;
        int _csPin;
        int _mode;
        bool _msbFirst;
        spi_device_handle_t _spiHandle;

    public:
        // El constructor recibe el hardware a usar
        SPI(spi_host_device_t spiHost, int mosi, int miso, int clk, int cs, int mode, bool msbFirst);
        
        void init();
        void writeSPI(uint8_t reg, uint8_t data);
        uint8_t readSPI(uint8_t reg);
};

#endif