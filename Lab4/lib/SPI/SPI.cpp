#include "SPI.h"
#include <string.h> 

SPI::SPI(spi_host_device_t spiHost, int mosi, int miso, int clk, int cs, int mode, bool msbFirst) {
    _spiHost = spiHost;
    _mosiPin = mosi;
    _misoPin = miso;
    _clkPin  = clk;
    _csPin   = cs;
    _mode = mode;
    _msbFirst = msbFirst; 

}

void SPI::init() {
    //Master config
    spi_bus_config_t buscfg = {
        .mosi_io_num = _mosiPin,
        .miso_io_num = _misoPin, 
        .sclk_io_num = _clkPin,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 32
    };
    spi_bus_initialize(_spiHost, &buscfg, SPI_DMA_CH_AUTO);

    //Slave Config
    spi_device_interface_config_t devcfg = {
        .mode = _mode, 
        .clock_speed_hz = 1000000, 
        .spics_io_num = _csPin,
        .flags = _msbFirst ? 0 : SPI_DEVICE_BIT_LSBFIRST,
        .queue_size = 1,
       
    };
    spi_bus_add_device(_spiHost, &devcfg, &_spiHandle);
}

void SPI::writeSPI(uint8_t reg, uint8_t data) {
    uint8_t txData[2];
    
    // Transmisión genérica: Byte 0 es el registro, Byte 1 es el dato
    txData[0] = reg; 
    txData[1] = data;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); 
    t.length = 16;            
    t.tx_buffer = txData;     

    spi_device_transmit(_spiHandle, &t);
}

uint8_t SPI::readSPI(uint8_t reg) {
    uint8_t txData[2];
    uint8_t rxData[2];
    
    // Lectura genérica: Byte 0 es el registro, Byte 1 empuja el reloj
    txData[0] = reg; 
    txData[1] = 0x00; 

    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 16;
    t.tx_buffer = txData;
    t.rx_buffer = rxData; 

    spi_device_transmit(_spiHandle, &t);

    return rxData[1]; 
}